/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/



class ResShader : public ResoureImpl<ResShader, RESID_SHADER, Allocators::default>
{
	static constexpr char *SubprogramTypes[] = { "Vert", "Frag", "Geom", "Ctrl", "Eval" };
	static constexpr SIZE_T NumSubprogramTypes = COUNT_OF(SubprogramTypes);
	static constexpr wchar_t *src[] = {
		L".vert.glsl", nullptr,
		L".frag.glsl", nullptr,
		L".geom.glsl", nullptr,
		L".tesc.glsl", nullptr,
		L".tese.glsl", nullptr,
		nullptr, nullptr };

	static constexpr wchar_t *bin[] = {
		L".vert.spv", nullptr,
		L".frag.spv", nullptr,
		L".geom.spv", nullptr,
		L".tesc.spv", nullptr,
		L".tese.spv", nullptr,
		nullptr, nullptr
	};

	// ----------------------------------------------------------------------------------

	ResourceBase *Res;
	U8 *Data;
	SIZE_T Size;

	bool isModified;
	bool isUpdateRecived;

	struct File {
		WSTR Filename;
		SIZE_T Size;
		time_t Timestamp;
		ResourceUpdateNotifyReciver<ResShader> UpdateNotifyReciver;
		bool IsEmpty() { return Filename.size() == 0; }

		/// Check in loaded resources or on disk. 
		/// Update Size & Timestamp if needed.
		bool IsModified() 
		{
			auto res = GetResource();
			if (res && res->isLoaded()) 
			{
				if (res->GetSize() != Size || res->GetTimestamp() != Timestamp)
				{
					Size = res->GetSize();
					Timestamp = res->GetTimestamp();
					return true;
				}
				return false;
			}

			if (Filename.size())
			{
				SIZE_T fSize = Size;
				time_t fTimestamp = Timestamp;
				bool fExist = Tools::InfoFile(&Size, &Timestamp, Filename);

				if (!fExist)
				{
					Size = 0;
					Timestamp = 0;
					UpdateNotifyReciver.StopNotifyReciver();
				}
				return !fExist || fSize != Size || fTimestamp != Timestamp;
			}
			return false;
		}

		ResourceBase *GetResource() { return  UpdateNotifyReciver.GetResource();  }
		void SetResource(ResourceBase *res, ResShader *owner) { UpdateNotifyReciver.SetResource(res, owner); }
		
	};

	//	File Subprograms[NumSubprogramTypes*2];
	File Source[NumSubprogramTypes];
	File Binary[NumSubprogramTypes];

	// ------------------------------------------------------------------------------------	

	bool _ParseFilename(const WSTR &prg, U32 *pStage, bool *pIsSrc = nullptr)
	{
		bool isSrc = prg.endsWith(L".glsl");
		bool isBin = isSrc ? false : prg.endsWith(L".spv"); // we don't check for ".spv" if it ends ".glsl"

		if (!isSrc && !isBin)
			return false;

		if (pIsSrc)
			*pIsSrc = isSrc;

		auto ends = isSrc ? src : bin;

		U32 stage = -1;
		bool ret = false;
		for (auto f = ends; *f && !ret; ++f)
		{
			for (; *f; ++f)
			{
				if (prg.endsWith(*f))
				{
					ret = true;
					break;
				}
			}
			++stage;
		}
		if (ret && pStage)
			*pStage = stage;

		return ret;
	}

	File * _FindFile(const WSTR &prg)
	{
		bool isSrc;
		U32 stage;
		if (_ParseFilename(prg, &stage, &isSrc))
			return isSrc ? &Source[stage] : &Binary[stage];

		return nullptr;
	}

	U32 _FindStage(const WSTR &prg)
	{
		U32 stage = -1;
		_ParseFilename(prg, &stage);

		return stage;
	}

public:
	ResShader(ResourceBase *res) : Res(res), Data(nullptr), Size(0), isModified(true), isUpdateRecived(false) {}
	~ResShader() { Release(Res); }
	void Release(ResourceBase *res)
	{
		Save(res);
		res->ResourceLoad(nullptr);
		if (Data)
			deallocate(Data);

		Data = nullptr;
		Size = 0;
	}

	File * AddProgram(WSTR filename)
	{
		auto rm = globalResourceManager;
		if (!Res->isLoaded() && !Res->isDeleted())
		{
			rm->LoadSync(Res);
		}

		ResourceBase *res = nullptr;
		if (auto p = _FindFile(filename))
		{
			rm->AbsolutePath(filename);
			if (p->Filename != filename)
			{
				isModified = true;
				res = rm->GetByFilename(filename, RawData::GetTypeId()); // create resource (if not exist)
				p->Filename = filename;
				p->SetResource(res, this);
				// Timestamp = 0 will trigger compilation of source code or build of final shader program
				p->Size = 0;
				p->Timestamp = 0;
			}
			return p;
		}

		return nullptr;
	}


	WSTR &GetSourceFilename(U32 stage)
	{
		assert(stage >= NumSubprogramTypes);
		return Source[stage].Filename;
	}


	WSTR &GetBinaryFilename(U32 stage)
	{
		assert(stage >= NumSubprogramTypes);
		return Binary[stage].Filename;
	}

	void Update(ResourceBase *res)
	{
		using tinyxml2::XMLPrinter;
		using tinyxml2::XMLDocument;
		isUpdateRecived = true;

		// memory is allocated with RawData MemoryAllocator, so we don't have to copy it.
		Data = static_cast<U8 *>(res->GetData());
		Size = res->GetSize();
		auto &rm = globalResourceManager;
		STR fn;
		WSTR cvt;
		if (Data)
		{
			XMLDocument xml;
			xml.Parse(reinterpret_cast<const char *>(Data), Size);

			auto root = xml.FirstChildElement("Shader");
			for (auto r = root->FirstChildElement("Program"); r; r = r->NextSiblingElement())
			{
				auto fileName = r->GetText();
				if (!fileName)
					continue;

				fn = fileName;
				cvt.UTF8(fn);
				rm->AbsolutePath(cvt);

				if (auto p = AddProgram(fn))
				{
					p->Timestamp = r->Int64Attribute("Update", 0);
					p->Size = r->Int64Attribute("Size", 0);
				}
			}
		}
		isModified = false;

		// step 2: make sure, if we have source, we need compiled program too.
		for (U32 stage = 0; stage < NumSubprogramTypes; ++stage)
		{
			auto &src = Source[stage];
			auto &bin = Binary[stage];
			if (src.IsEmpty() && bin.IsEmpty())
				continue; // nothing to do with empty source and binary at i-stage

			if (!src.IsEmpty() && bin.IsEmpty())
			{
				Compile(&src);
			}

			if (!src.IsEmpty() && !src.IsEmpty())
			{
				if (src.IsModified())
				{
					isModified = true;
					if (src.Timestamp == 0 && src.Size == 0)
					{
						Delete(&src);
					}
					else
					{
						Compile(&src);
					}
				}
			}
		}
	}


	void Save(ResourceBase *res)
	{
		if (!isModified && isUpdateRecived)
			return;

		using tinyxml2::XMLPrinter;
		using tinyxml2::XMLDocument;

		auto &rm = globalResourceManager;
		XMLDocument out;
		auto *root = out.NewElement("Shader");
		STR cvt;
		auto writeToXML = [&](File &prg) {
			if (prg.IsEmpty()) // don't add to XML empty entries
				return;

			auto *entry = out.NewElement("Program");

			// we need relative path and Filename is absolute
			WSTR fpath = prg.Filename;
			rm->RelativePath(fpath);

			// we need UTF8 (to put in XML)
			cvt.UTF8(fpath);

			entry->SetAttribute("Update", prg.Timestamp);
			entry->SetAttribute("Size", static_cast<I64>(prg.Size));
			entry->SetText(cvt.c_str());
			root->InsertEndChild(entry);
		};

		for (auto &prg : Source)
			writeToXML(prg);
		for (auto &prg : Binary)
			writeToXML(prg);


		out.InsertFirstChild(root);
		XMLPrinter prn;
		out.Print(&prn);

		if (res->Path.size() == 0)
		{
			// build new file name
			int fidx = 1;
			WSTR fn = res->Name;
			rm->AbsolutePath(fn);
			WSTR fnWithExt = fn + L".shader";
			while (Tools::Exist(fnWithExt))
			{
				++fidx;
				fnWithExt = fn + fidx + L".shader";
			}
			res->Path = fnWithExt;
		}

		Tools::SaveFile(prn.CStrSize() - 1, prn.CStr(), res->Path);

		// file saved... but res may be marked as deleted (not file on disk befor save) or with old file time stamp
		rm->RefreshResorceFileInfo(res);
	}

	U32 GetBinaryCount()
	{
		U32 cnt = 0;
		for (auto &p : Binary)
			if (!p.IsEmpty())
				++cnt;

		return cnt;
	}

	U32 GetSourceCount()
	{
		U32 cnt = 0;
		for (auto &p : Binary)
		{
			if (!p.IsEmpty())
				++cnt;
		}

		return cnt;
	}

	ResourceBase *GetBinary(U32 idx)
	{
		for (auto &p : Binary)
		{
			if (!p.IsEmpty())
			{
				if (idx == 0)
					return p.UpdateNotifyReciver.GetResource();
				--idx;
			}
		}

		return nullptr;
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }

	void Compile(File *p)
	{
		// find matching stage;
		U32 stage = 0;
		File *o = nullptr;
		for (auto &src : Source)
		{
			if (&src == p)
			{
				o = &Binary[stage];
				break;
			}
			++stage;
		}

		assert(o);


		if (o)
		{
			TRACEW(L"compile: " << p->Filename.c_str() << L"\n");
			WSTR binFilename = o->Filename;
			if (binFilename.size() == 0)
			{
				binFilename = p->Filename;
				binFilename.resize(binFilename.size() - 4);
				binFilename += L"spv";
			}

			WSTR cmd = L"glslangValidator.exe -V ";
			cmd += p->Filename + L" -o " + binFilename;
			if (Tools::WinExec(cmd) == 0)
			{
				AddProgram(binFilename);				
			}
			else
			{
				TRACEW(L"compile error\n");
			}
		}
	}

	PRELOAD_SHADER reloadShaderMessageData;
	void Link(File *p)
	{		
		TRACEW(L"link: " << p->Filename.c_str() << L"\n");
		reloadShaderMessageData.wnd = -1;
		reloadShaderMessageData.shader = Res->Name.c_str();
		CORE::Message msg = { RELOAD_SHADER, RENDERING_ENGINE, 0, &reloadShaderMessageData };
		BAMS::CORE::IEngine::PostMsg(&msg, 500ms);
	}

	void Delete(File *p)
	{
		TRACEW(L"deleted: " << p->Filename.c_str() << L"\n");
		p->Filename = "";
	}

	/// <summary>
	/// Recive notification about source or compiled shader programs.
	/// </summary>
	/// <param name="res">The resource (with shader program).</param>
	/// <param name="type">The type.</param>
	void Notify(ResourceBase *res, U64 type)
	{
		bool isResModified = false;
		auto getFile = [&](auto &files) {
			for (auto &f : files)
			{
				if (!f.IsEmpty() && f.GetResource() == res)
				{
					isResModified = f.IsModified();
					return &f;
				}
			}
			return (File *)nullptr;
		};
		auto src = getFile(Source);
		auto bin = getFile(Binary);

		assert(src || bin);
		if (!src && !bin)
			return;

		if (isResModified)
		{
			if (res->isDeleted())
				Delete(src ? src : bin);

			if (src)
				Compile(src);

			// we don't need to link after Notify, only source compilation may trigger link... hmm
			if (bin)
				Link(bin);

			isModified = true;
		}
	}
};
