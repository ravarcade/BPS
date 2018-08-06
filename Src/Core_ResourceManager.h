/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResourceBase;
class ResourceInterface;
class ResourceFactoryInterface;
class ResourceManager;

class ResourceInterface
{
public:
	virtual ~ResourceInterface() {}
	virtual void Update(ResourceBase *) = 0;
	virtual ResourceFactoryInterface *GetFactory() = 0;
};

class ResourceFactoryInterface
{
public:
	U32 TypeId;
	virtual ~ResourceFactoryInterface() {}
	virtual ResourceInterface *Create() = 0;
	virtual void Destroy(void *ptr) = 0;
	virtual IMemoryAllocator *GetMemoryAllocator() = 0;
};

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
	ResourceInterface *_resourceImplementation;

public:
	UUID UID;
	U32  Type;
	STR  Name;
	WSTR Path;

	ResourceBase() : _resourceImplementation(nullptr), _isLoaded(false), _refCounter(0), Type(UNKNOWN), UID(Tools::NOUID) {};
	virtual ~ResourceBase() {

		if (_resourceImplementation)
		{
			_resourceImplementation->GetFactory()->Destroy(_resourceImplementation);
		}
	};

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
	ResourceInterface *GetImplementation() { return _resourceImplementation; }

	void AddRef() { ++_refCounter; }
	void Release() { --_refCounter; }

	enum {
		UNRECOGNIZED = -1,
		UNKNOWN = 0,
	};

	void Update() { if (_resourceImplementation) _resourceImplementation->Update(this); }
	IMemoryAllocator *GetMemoryAllocator() { return !_resourceImplementation ? nullptr : _resourceImplementation->GetFactory()->GetMemoryAllocator(); }

	friend class ResourceManager;
};

class ResourceFactoryChain : public ResourceFactoryInterface
{
public:
	static ResourceFactoryChain *First;
	ResourceFactoryChain *Next;

	ResourceFactoryChain()
	{ 
		Next = First; 
		First = this; 
	}
};

template <typename T, U32 ResTypeId, class Alloc = Allocators::Default>
class ResoureImpl : public ResourceInterface, public MemoryAllocatorStatic<Alloc>
{
public:
	class ResFactory : public ResourceFactoryChain, public MemoryAllocatorStatic<Alloc>
	{
	public:
		ResourceInterface *Create() { return make_new<T>(); }
		void Destroy(void *ptr) { make_delete<T>(ptr); }
		IMemoryAllocator *GetMemoryAllocator() { return MemoryAllocatorStatic<Alloc>::GetMemoryAllocator(); }
		ResFactory() : ResourceFactoryChain() {
			TypeId = ResTypeId; 
		}
	};
	ResourceFactoryInterface *GetFactory() { return &_resourceFactory; }

	static ResFactory _resourceFactory;
	static U32 GetTypeId() { return _resourceFactory.TypeId; }
};

template <typename T, U32 ResTypeId, class Alloc>
typename ResoureImpl<T, ResTypeId, Alloc>::ResFactory ResoureImpl<T, ResTypeId, Alloc>::_resourceFactory;


// ============================================================================

class BAMS_EXPORT ResourceManager : public MemoryAllocatorStatic<>
{
private:
	struct InternalData;
	InternalData *_data;

public:
	ResourceManager();
	~ResourceManager();

	static ResourceManager *Create();
	static void Destroy(ResourceManager *);

	void Filter(ResourceBase **resList, U32 *resCount, CSTR &patern);
	ResourceBase *Get(const STR &resName, U32 typeId = ResourceBase::UNKNOWN);
	ResourceBase *Get(CSTR resName, U32 typeId = ResourceBase::UNKNOWN) { return Get(STR(resName), typeId); };
	ResourceBase *Get(const UUID &resUID);

	template<class T>
	ResourceBase *Get(const STR &resName) { return Get(resName, T::GetTypeId()); }

	template<class T>
	ResourceBase *Get(CSTR resName) { return Get(resName, T::GetTypeId()); }

	void Add(CWSTR path);
	void AddDir(CWSTR path);

	void LoadSync();
	void LoadAsync();

	void StartDirectoryMonitor();
	void StopDirectoryMonitor();
};


// ============================================================================

class ResourceManagerModule : IModule
{
public:
	void Update(float dt);
	void Initialize();
	void Finalize();
	void SendMessage(Message *msg);
	~ResourceManagerModule();
};


#define RESOURCEMANAGER_ADD_FILE 0x10001