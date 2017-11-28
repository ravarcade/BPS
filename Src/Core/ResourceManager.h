/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/


/// <summary>
/// Base Resource Class.
/// It provides common for all resources field, like pointer to memory with data, length, state if it is loaded to mem.
/// </summary>
class ResourceBase 
{
protected:

	void *_resourceData;
	SIZE_T _resourceSize;
	bool _isLoaded;
	U32 _refCounter;

public:
	UUID UID;
	U32  Type;
	STR  Name;
	PathSTR Path;

	ResourceBase(CSTR path, CSTR name = nullptr);
	ResourceBase() {};


	void ResourceLoad(void *data, SIZE_T size)
	{
		_resourceData = data;
		_resourceSize = size;
		_isLoaded = false;
		_refCounter = 0;
	}

	bool isLoaded() { return _isLoaded; }
	void *GetData() { return _resourceData; }
	SIZE_T GetSize() { return _resourceSize; }
	void AddRef() { ++_refCounter; }
	void Release() { --_refCounter; }

	enum {
		UNRECOGNIZED = -1,
		UNKNOWN = 0,
	};
};


struct ResourceTypeListEntry
{
	typedef void *(*CreatePtr)(ResourceBase *);
	typedef void(*DestroyPtr)(void *);

	U32 TypeId;
	CreatePtr Create;
	DestroyPtr Destroy;

	ResourceTypeListEntry(U32 id, CreatePtr pCreate, DestroyPtr pDestroy) : TypeId(id), Create(pCreate), Destroy(pDestroy) {}
};

static std::vector<ResourceTypeListEntry> RegistrationList;

template<typename T>
class ResourceTypeRegistration
{
public:
	ResourceTypeRegistration() { 
		RegistrationList.push_back(ResourceTypeListEntry(T::ResTypeId, T::Create, T::Destroy)); 
	}
};

template <typename T, int ResTypeId, class Alloc = Allocators::Default>
class ResoureImpl : public MemoryAllocatorImpl<Alloc>
{
public:
	// required methods to work as resource
	static void *Create(ResourceBase *res) { return make_new<T>(); };
	static void Destroy(void *ptr) { make_delete<T>(ptr); }
	const U32 ResourceTypeId = ResTypeId;
	static ResourceTypeRegistration<T> _AutomaticRegister;
};

template <typename T, int ResTypeId, class Alloc>
ResourceTypeRegistration<T> ResoureImpl<T, ResTypeId, Alloc>::_AutomaticRegister;

class RawData : public ResoureImpl<RawData, 0x00010001, Allocators::Default>
{
public:
	U8 *Data;
	SIZE_T Size;

	RawData() : Data(nullptr), Size(0) {}
	~RawData() { if (Data) deallocate(Data); }
	void Update(ResourceBase *res) 
	{
		Size = res->GetSize();
		if (Size)
		{
			Data = static_cast<U8 *>( allocate(Size) );
			memcpy_s(Data, Size, res->GetData(), Size);
		}
	}
};


//#define REGRESOURCE(x)

//REGRESOURCE(RawData);


template<class T>
class Resource : public ResourceBase
{
private:
	T *_resourceImplementation;

public:
	Resource() { _resourceImplementation = T::Create(this); Type = T::ResourceTypeId; }
	virtual ~Resource() { if (_resourceImplementation) T::Destroy(_resourceImplementation); }
	void Update() { _resourceImplementation->Update(this); }

	
};

class BAMS_EXPORT ResourceManager : public MemoryAllocatorImpl<>
{
private:
	struct InternalData;
	InternalData *_data;

	void _Add(ResourceBase *res);

	template<class T>
	Resource<T> *_New(const STR &resName, const UUID &resUID)
	{
		auto t = make_new<Resource<T> >();
		t->Name = resName;
		t->UID = resUID;
		_Add(t);
		return t;
	}

public:
	ResourceManager();
	~ResourceManager();

	//		void Import(ResourceManifest *pResourceManifest, int importType);

	ResourceBase *Find(const STR &resName);
	ResourceBase *Find(CSTR resName) { return Find(STR(resName)); };
	ResourceBase *Find(const UUID &resUID);

	void Add(CSTR path, CSTR name = nullptr);

	template<class T>
	Resource<T> *Get(const STR &resName) { auto res = Find(resName); return res != nullptr ? static_cast<Resource<T> *>(res) : _New<T>(resName, Tools::NOUID); }

	template<class T>
	Resource<T> *Get(CSTR resName) { auto res = Find(resName); return res != nullptr ? static_cast<Resource<T> *>(res) : _New<T>(resName, Tools::NOUID); }

	template<class T>
	Resource<T> *Get(const UUID &resUID) { auto res = Find(resUID); return res != nullptr ? static_cast<Resource<T> *>(res) : _New<T>("", resUID); }
};
