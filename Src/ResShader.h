/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/



class ResShader : public ResoureImpl<ResShader, RESID_SHADER, Allocators::default>
{
	static constexpr U32 NOT_FOUND = -1;
	static constexpr char *SubprogramTypes[] = { "Vert", "Frag", "Geom", "Ctrl", "Eval" };
	static constexpr SIZE_T NumSubprogramTypes = COUNT_OF(SubprogramTypes);

	U32 FindType(const WSTR &prg)
	{
		if (prg.endsWith(L".glsl") || prg.endsWith(L".spv"))
		{
			static const wchar_t *ends[] = {
				L".vert.glsl", nullptr,
				L".frag.glsl", nullptr,
				L".geom.glsl", nullptr,
				L".tesc.glsl", nullptr,
				L".tese.glsl", nullptr,

				L".vert.spv", nullptr,
				L".frag.spv", nullptr,
				L".geom.spv", nullptr,
				L".tesc.spv", nullptr,
				L".tese.spv", nullptr,
				nullptr, nullptr
			};

			U32 type = 0;
			for (const wchar_t **f = ends; *f; ++f)
			{
				for (; *f; ++f)
				{
					if (prg.endsWith(*f))
						return type;
				}
				++type;
			}
		}

		return NOT_FOUND;
	}

public:
	U8 *Data;
	SIZE_T Size;

	bool isModified;
	bool isUpdateRecived;

	struct File {
		WSTR FileName;
		SIZE_T Size;
		time_t Timestamp;
		ResourceUpdateNotifyReciver<ResShader> UpdateNotifyReciver;
		bool IsEmpty() { return FileName.size() == 0; }
	};



	File Subprograms[NumSubprogramTypes*2];
	ResourceBase *Res;
	ResShader(ResourceBase *res) : Res(res), Data(nullptr), Size(0), isModified(true), isUpdateRecived(false) {}
	~ResShader() { if (Data) deallocate(Data); }

	U32 AddProgram(WSTR prg)
	{
		auto rm = globalResourceManager;
		if (!Res->isLoaded() && !Res->isDeleted())
		{
			rm->LoadSync(Res);
		}

		ResourceBase *res = nullptr;
		U32 type = FindType(prg);
		if (type != NOT_FOUND) 
		{
			auto &p = Subprograms[type];
			rm->AbsolutePath(prg);
			if (p.FileName == prg)
				return type;

			isModified = true;
			res = rm->GetByFilename(prg, RawData::GetTypeId());
			p.FileName = prg;
			p.UpdateNotifyReciver.SetResource(res, this, type);
			// Timestamp = 0 will trigger compilation of source code or build of final shader program
			p.Size = 0;
			p.Timestamp = 0;
		}
		return type;
	}


	WSTR &GetSubprogramName(U32 type)
	{
		assert(type >= NumSubprogramTypes);
		auto &p = Subprograms[type];
		return p.FileName;
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

				U32 type = AddProgram(fn);
				if (type != NOT_FOUND)
				{
					auto &p = Subprograms[type];
					p.Timestamp = r->Int64Attribute("Update", 0);
					p.Size = r->Int64Attribute("Size", 0);
				}
			}
		}
		isModified = false;

		// step 2: make sure, if we have source, we need compiled program too.
		for (U32 i = 0; i < NumSubprogramTypes; ++i)
		{
			if (!Subprograms[i].IsEmpty() && Subprograms[i + NumSubprogramTypes].IsEmpty())
			{		
				Compile(&Subprograms[i]);
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
		for (auto &prg : Subprograms)
		{
			if (prg.IsEmpty()) // don't add to XML empty entries
				continue;

			auto *entry = out.NewElement("Program");

			// we need relative path and FileName is absolute
			WSTR fpath = prg.FileName;
			rm->RelativePath(fpath);

			// we need UTF8 (to put in XML)
			cvt.UTF8(fpath);

			entry->SetAttribute("Update", prg.Timestamp);
			entry->SetAttribute("Size", static_cast<I64>(prg.Size));
			entry->SetText(cvt.c_str());
			root->InsertEndChild(entry);
		}

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

	U32 GetSubprogramsCount()
	{
		U32 cnt = 0;
		for (U32 i = 0; i < NumSubprogramTypes; ++i)
		{
			if (!Subprograms[i + NumSubprogramTypes].IsEmpty())
				++cnt;
		}

		return cnt;
	}

	ResourceBase *GetSubprogram(U32 idx)
	{
		for (U32 i = 0; i < NumSubprogramTypes; ++i)
		{
			if (!Subprograms[i + NumSubprogramTypes].IsEmpty())
			{
				if (idx == 0)
					return Subprograms[i + NumSubprogramTypes].UpdateNotifyReciver.GetResource();
				--idx;
			}
		}

		return nullptr;
	}

	void Release(ResourceBase *res)
	{
		Save(res);
		res->ResourceLoad(nullptr);
		if (Data)
			res->GetMemoryAllocator()->deallocate(Data);

		Data = nullptr;
		Size = 0;
	}

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }

	void Compile(File *p)
	{
		auto t = FindType(p->FileName);
		if (t < NumSubprogramTypes)
		{
			TRACEW(L"compile: " << p->FileName.c_str() << L"\n");
			auto &o = Subprograms[t + NumSubprogramTypes];
			WSTR out = o.FileName;
			if (out.size() == 0)
			{
				out = p->FileName;
				out.resize(out.size() - 4);
				out += L"spv";
			}

			WSTR cmd = L"glslangValidator.exe -V ";
			cmd += p->FileName + L" -o " + out;
			if (Tools::WinExec(cmd) == 0)
			{
				AddProgram(out);
			}
			else
			{
				TRACEW(L"compile error\n");
			}
		}
	}

	void Link(File *p)
	{
		auto t = FindType(p->FileName);
		if (t >= NumSubprogramTypes)
		{
			TRACEW(L"link: " << p->FileName.c_str() << L"\n");
		}
	}

	void Notify(ResourceBase *res, U64 type)
	{
		assert(type < static_cast<U64>(COUNT_OF(Subprograms)));

		// don't be depend on "type" ... find it base on res
		File *pPrg = nullptr;
		for (U32 i = 0; i < COUNT_OF(Subprograms); ++i)
		{
			if (!Subprograms[i].IsEmpty() && Subprograms[i].UpdateNotifyReciver.GetResource() == res)
			{
				pPrg = &Subprograms[i];
				break;
			}
		}
		assert(pPrg != nullptr);
		if (pPrg == nullptr)
			return;

		if (res->isDeleted())
		{
			TRACEW(L"deleted: " << pPrg->FileName.c_str() << L"\n");
		}

		if (pPrg->Size != res->GetSize() || pPrg->Timestamp != res->GetFileTimestamp())
		{
			pPrg->Size = res->GetSize();
			pPrg->Timestamp = res->GetFileTimestamp();
			isModified = true;

			Compile(pPrg);
			Link(pPrg);
		}
	}	
};
