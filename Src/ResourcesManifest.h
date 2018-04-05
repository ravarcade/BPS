/*
namespace BAMS {

	struct ResourceSourceDescription : public CORE::ExtraMemory<ResourceSourceDescription>
	{
		UUID UID;
		U32  Type;
		relative_ptr<CSTR> Name;
		relative_ptr<CSTR> Path;
		SIZE_T TotalLen;
	};

	enum ResourceType {
		UNKNOWN = 0
	};

	enum ImportType {
		MOVE = 0,
		COPY = 1
	};

	class BAMS_EXPORT ResourceManifest
	{
		struct InternalData;
		InternalData *_data;

	public:
		ResourceManifest();
		~ResourceManifest();

		ResourceSourceDescription *Create(CSTR path, CSTR name = nullptr);
		void Remove(ResourceSourceDescription *resDesc);

		ResourceSourceDescription *Find(UUID &&uid);

		void Add(ResourceManifest &resourceManifest);
	};

};
*/