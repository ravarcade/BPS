/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

typedef TResRawData<RESID_SHADER_SRC, false> ResShaderSrc;
typedef TResRawData<RESID_SHADER_BIN, true> ResShaderBin;


class ResShader : public ResImp<ResShader, RESID_SHADER>
{
	static constexpr char *SubprogramTypes[] = { "Vert", "Frag", "Geom", "Ctrl", "Eval" };
	static constexpr SIZE_T NumSubprogramTypes = COUNT_OF(SubprogramTypes);
	static constexpr wchar_t *src[] = {
		L".vert.glsl", L".vert", nullptr,
		L".frag.glsl", L".frag", nullptr,
		L".geom.glsl", L".geom", nullptr,
		L".tesc.glsl", L".tesc", nullptr,
		L".tese.glsl", L".tese", nullptr,
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

		ResBase *GetResource() { return  UpdateNotifyReciver.GetResource();  }
		void SetResource(ResBase *res, ResShader *owner) { UpdateNotifyReciver.SetResource(res, owner); }
		
	};

	File Source[NumSubprogramTypes];
	File Binary[NumSubprogramTypes];

	// ------------------------------------------------------------------------------------	

	bool _ParseFilename(const WSTR &prg, U32 *pStage, bool *pIsSrc = nullptr);

	void _LoadXML();
	void _SaveXML();

public:
	ResBase *rb;
	ResShader(ResBase *res) : rb(res), isModified(true), isUpdateRecived(false) {}
	~ResShader() { 
//		TRACE("~ResShader: " << rb->Name.c_str() << "\n");
		Release(rb); 
	}

	void Release(ResBase *res)
	{
		assert(rb = res);
		Save(res);
		res->ReleaseMemory();
	}

	U8 *GetData() { return rb ? static_cast<U8 *>(rb->GetData()) : nullptr; }
	SIZE_T GetSize() { return rb ? rb->GetSize() : 0; }

	File * AddProgram(WSTR filename);
	WSTR &GetSourceFilename(U32 stage);
	WSTR &GetBinaryFilename(U32 stage);
	void Update(ResBase *res);
	void Save(ResBase *res);

	U32 GetBinaryCount();
	U32 GetSourceCount();
	ResBase *GetBinary(U32 idx);

	void Compile(File *p);

	PRELOAD_SHADER reloadShaderMessageData;
	void Link(File *p);
	void Delete(File *p);
	void Notify(ResBase *res, U32 type);

	static void IdentifyResourceType(ResBase *res);
};
