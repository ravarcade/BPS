/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResourceBase;
class ResourceImplementationInterface;
class ResourceFactoryInterface;
class ResourceManager;

/// <summary>
/// Interface to resource implementation
/// </summary>
class ResourceImplementationInterface
{
public:
	virtual ~ResourceImplementationInterface() {}
	virtual void Update(ResourceBase *) = 0;
	virtual void Release(ResourceBase *) = 0;
	virtual ResourceFactoryInterface *GetFactory() = 0;
};

/// <summary>
/// Interfec to resource factory.
/// Every resource type has own resource factory.
/// Every factory has:
/// - Create - object constructor.
/// - Destroy - object destructor.
/// - GetMemoryAllocator() - memory allocator. Resource loader will use it to reserve memory for resource data.
/// - TypeId - U32 type id.
/// </summary>
class ResourceFactoryInterface
{
public:
	U32 TypeId;
	virtual ~ResourceFactoryInterface() {}
	virtual ResourceImplementationInterface *Create() = 0;
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
	bool _isDeleted;
	bool _isModified;
	time _waitWithUpdate;
	U32 _refCounter;

	ResourceImplementationInterface *_resourceImplementation;
	const static time::duration defaultDelay;

public:
	UUID UID;
	U32  Type;
	STR  Name;
	WSTR Path;
	
	/// <summary>
	/// Initializes a new instance of the <see cref="ResourceBase"/> class.
	/// It is empty resource without assigned any file on disk.
	/// </summary>
	ResourceBase() :
		_isLoaded(false), 
		_isDeleted(false),
		_isModified(false),
		_refCounter(0),
		_resourceImplementation(nullptr),
		Type(UNKNOWN),
		UID(Tools::NOUID) 
	{};

	virtual ~ResourceBase()
	{
		if (_resourceImplementation)
			_resourceImplementation->GetFactory()->Destroy(_resourceImplementation);
	};

	void Init(CWSTR path);

	void ResourceLoad(void *data, SIZE_T size) 
	{
		_resourceData = data;
		_resourceSize = size;
		_isLoaded = data && size;
	}

	inline bool isLoaded() { return _isLoaded; }
	inline bool isDeleted() { return _isDeleted; }
	inline bool isModified() { return _isModified; }

	void *GetData() { return _resourceData; }
	SIZE_T GetSize() { return _resourceSize; }
	ResourceImplementationInterface *GetImplementation() { return _resourceImplementation; }

	void AddRef()
	{ 
		++_refCounter; 
	}

	void Release() 
	{ 
		--_refCounter; 
		if (_refCounter == 0 && _resourceImplementation != nullptr)
		{	// resource not used (with _refCounter == 0) don't need resource implementation... "Destroy" will delete it.
			_resourceImplementation->GetFactory()->Destroy(_resourceImplementation);
			_resourceImplementation = nullptr;
		}
	}

	U32 GetRefCounter() { return _refCounter; }

	enum {
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


/// <summary>
/// Helper class used to make resource implementation easier.
/// It will add ResFactory to class.
/// </summary>
template <typename T, U32 ResTypeId, class Alloc = Allocators::Default>
class ResoureImpl : public ResourceImplementationInterface, public MemoryAllocatorStatic<Alloc>
{
public:
	class ResFactory : public ResourceFactoryChain, public MemoryAllocatorStatic<Alloc>
	{
	public:
		ResourceImplementationInterface *Create() { return make_new<T>(); }
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

	void CreateResourceImplementation(ResourceBase *res);

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

	ResourceBase *Add(CWSTR path);
	void AddDir(CWSTR path);

	void LoadSync();
	void LoadAsync();

	void StartDirectoryMonitor();
	void StopDirectoryMonitor();
};


// ============================================================================

class ResourceManagerModule : public IModule
{
public:
	void Update(float dt);
	void Initialize();
	void Finalize();
	void SendMessage(Message *msg);
	~ResourceManagerModule();

	U32 GetModuleId() { return IModule::ResourceManagerModule; }

	ResourceManager *GetResourceManager();

};


#define RESOURCEMANAGER_ADD_FILE 0x10001