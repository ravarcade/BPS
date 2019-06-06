/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

typedef TResRawData<RESID_SHADER_SRC> ResShaderSrc;
typedef TResRawData<RESID_SHADER_BIN> ResShaderBin;

class ResShader : public ResoureImpl<ResShader, RESID_SHADER>
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

	ResourceBase *rb;
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

	File Source[NumSubprogramTypes];
	File Binary[NumSubprogramTypes];

	// ------------------------------------------------------------------------------------	

	bool _ParseFilename(const WSTR &prg, U32 *pStage, bool *pIsSrc = nullptr);

	void _LoadXML();
	void _SaveXML();

public:
	ResShader(ResourceBase *res) : rb(res), Data(nullptr), Size(0), isModified(true), isUpdateRecived(false) {}
	~ResShader() { Release(rb); }

	U8 *GetData() { return Data; }
	SIZE_T GetSize() { return Size; }

	void Release(ResourceBase *res)
	{
		Save(res);
		res->ResourceLoad(nullptr);
		if (Data)
			deallocate(Data);

		Data = nullptr;
		Size = 0;
	}

	File * AddProgram(WSTR filename);
	WSTR &GetSourceFilename(U32 stage);
	WSTR &GetBinaryFilename(U32 stage);
	void Update(ResourceBase *res);
	void Save(ResourceBase *res);

	U32 GetBinaryCount();
	U32 GetSourceCount();
	ResourceBase *GetBinary(U32 idx);


	void Compile(File *p);

	PRELOAD_SHADER reloadShaderMessageData;
	void Link(File *p);
	void Delete(File *p);
	void Notify(ResourceBase *res, U64 type);

	static void IdentifyResourceType(ResourceBase *res);
};
