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
	WSTR Path;

//	ResourceBase(CWSTR path, CSTR name = nullptr);
	ResourceBase() {};
	virtual ~ResourceBase() {};

	void Init(CWSTR path);

	void ResourceLoad(void *data, SIZE_T size)
	{
		_resourceData = data;
		_resourceSize = size;
		_isLoaded = data && size;
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

	virtual void Update() = 0;
	virtual IMemoryAllocator *GetMemoryAllocator() = 0;
};


struct ResourceTypeListEntry
{
	typedef void *(*CreatePtr)(ResourceBase *);
	typedef void  (*DestroyPtr)(void *);

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
class ResoureImpl : public MemoryAllocatorGlobal<Alloc>
{
public:
	// required methods to work as resource
	static void *Create(ResourceBase *res) { return make_new<T>(); };
	static void Destroy(T *ptr) { make_delete<T>(ptr); }
	static const U32 ResourceTypeId = ResTypeId;
	static ResourceTypeRegistration<T> _AutomaticRegister;
};

template <typename T, int ResTypeId, class Alloc>
ResourceTypeRegistration<T> ResoureImpl<T, ResTypeId, Alloc>::_AutomaticRegister;

template<class T>
class Resource : public ResourceBase
{
private:
	T *_resourceImplementation;

public:
	Resource() { _resourceImplementation = static_cast<T*>(T::Create(this)); Type = T::ResourceTypeId; }
	virtual ~Resource() { if (_resourceImplementation) T::Destroy(_resourceImplementation); }
	void Update() { _resourceImplementation->Update(this); }
	IMemoryAllocator *GetMemoryAllocator() { return _resourceImplementation->GetMemoryAllocator();  }
	T *GetResource() { return _resourceImplementation; }
};

class BAMS_EXPORT ResourceManager : public MemoryAllocatorGlobal<>
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

	static ResourceManager *Create();
	static void Destroy(ResourceManager *);

	ResourceBase *Find(const STR &resName, U32 typeId = ResourceBase::UNKNOWN);
	ResourceBase *Find(CSTR resName, U32 typeId = ResourceBase::UNKNOWN) { return Find(STR(resName), typeId); };
	ResourceBase *Find(const UUID &resUID);

	void Add(CWSTR path);

	void LoadSync();
	void LoadAsync();

	template<class T>
	Resource<T> *Get(const STR &resName) 
	{ 
		auto res = Find(resName, T::ResourceTypeId);
		if (res == nullptr || res->Type != T::ResourceTypeId)
			res = _New<T>(resName, Tools::NOUID);
		return static_cast<Resource<T> *>(res);
	}

	template<class T>
	Resource<T> *Get(CSTR resName) 
	{ 
		auto res = Find(resName, T::ResourceTypeId);
		if (res == nullptr || res->Type != T::ResourceTypeId)
		{
			auto res2 = _New<T>(resName, Tools::NOUID);
			res = res2;
		}
		return static_cast<Resource<T> *>(res);
	}

	template<class T>
	Resource<T> *Get(const UUID &resUID) 
	{ 
		auto res = Find(resUID); 
		if (res == nullptr || res->Type != T::ResourceTypeId)
		{
			auto res2 = _New<T>("", resUID);
			res = res2;
		}
		return static_cast<Resource<T> *>(res);
	}
};
