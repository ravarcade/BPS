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

class ResourceUpdateNotifier
{
public:
	struct IChain {
		IChain *next;
		IChain *prev;
		bool isValide;
		virtual void Notify() = 0;
	} *first;


	ResourceUpdateNotifier() : first(nullptr) {}
	~ResourceUpdateNotifier()
	{
		for (IChain *f = first; f; f = f->next)
			f->isValide = false;
	}

	void Notify()
	{
		for (IChain *f = first; f; f = f->next)
			f->Notify();
	}

	void Add(IChain *n)
	{
		n->next = first;
		n->prev = nullptr;
		if (first)
			first->prev = n;
		first = n;
		n->isValide = true;
	}

	void Remove(IChain *n)
	{
		if (n->next)
			n->next->prev = n->prev;

		if (first == n)
			first = n->next;
		else 
			n->prev->next = n->next;
	}
};

template<typename T>
class ResourceUpdateNotifyReciver : public ResourceUpdateNotifier::IChain
{
private:
	ResourceBase *_res;
	U64 _param;
	T *_owner;
public:
	ResourceUpdateNotifyReciver() : _res(nullptr), _param(0) {}
	~ResourceUpdateNotifyReciver() { StopNotifyReciver(); }
	void SetResource(ResourceBase *res, T *owner, U64 param = 0) 
	{ 
		StopNotifyReciver();
		_res = res; 
		_owner = owner; 
		_param = param; 
		_res->AddNotifyReciver(this); 
		res->AddRef();
	}
	
	void StopNotifyReciver() 
	{
		if (_res) {
			if (isValide)
				_res->RemoveNotifyReciver(this);
			_res->Release();
		}
	}

	void Notify() { if (_owner) _owner->Notify(_res, _param); }
	ResourceBase *GetResource() { return _res; }
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
	virtual ResourceImplementationInterface *Create(ResourceBase *res) = 0;
	virtual void Destroy(void *ptr) = 0;
	virtual IMemoryAllocator *GetMemoryAllocator() = 0;
	virtual bool IsLoadable() = 0;
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
	time_t _resourceTimestamp;
	ResourceImplementationInterface *_resourceImplementation;
	U32 _refCounter;

	bool _isLoaded;
	bool _isDeleted;
	bool _isModified;
	bool _isLoadable;
	time _waitWithUpdate;

	static constexpr time::duration defaultDelay = 200ms;
	ResourceUpdateNotifier _updateNotifier;

public:
	UUID UID;
	U32  Type;
	STR  Name;
	WSTR Path;
	STR XML;
	
	/// <summary>
	/// Initializes a new instance of the <see cref="ResourceBase"/> class.
	/// It is empty resource without assigned any file on disk.
	/// </summary>
	ResourceBase() :
		_isLoaded(false), 
		_isDeleted(false),
		_isModified(false),
		_isLoadable(true),
		_refCounter(0),
		_resourceImplementation(nullptr),
		Type(RESID_UNKNOWN),
		UID(Tools::NOUID)
	{};

	~ResourceBase()
	{
		if (_resourceImplementation)
			_resourceImplementation->GetFactory()->Destroy(_resourceImplementation);
	};

	void Init(CWSTR path);

	void ResourceLoad(void *data, SIZE_T size = -1) 
	{
		_resourceData = data;
		if (size != -1)
			_resourceSize = size;

		_isLoaded = data != nullptr;
	}

	inline bool isLoaded() { return _isLoaded; }
	inline bool isDeleted() { return _isDeleted; }
	inline bool isModified() { return _isModified; }
	inline bool isLoadable() { return _isLoadable; }

	void *GetData() { return _resourceData; }
	SIZE_T GetSize() { return _resourceSize; }
	time_t GetTimestamp() { return _resourceTimestamp; }
	ResourceImplementationInterface *GetImplementation() { return _resourceImplementation; }

	void AddRef() { ++_refCounter; }
	U32 GetRefCounter() { return _refCounter; }

	void Release() 
	{ 
		--_refCounter; 
		if (_refCounter == 0 && _resourceImplementation != nullptr)
		{	// resource not used (with _refCounter == 0) don't need resource implementation... "Destroy" will delete it.
			_resourceImplementation->Release(this);
			_resourceImplementation->GetFactory()->Destroy(_resourceImplementation);
			_resourceImplementation = nullptr;
		}
	}


	void Update() { if (_resourceImplementation) _resourceImplementation->Update(this); _updateNotifier.Notify(); }
	IMemoryAllocator *GetMemoryAllocator() { return !_resourceImplementation ? nullptr : _resourceImplementation->GetFactory()->GetMemoryAllocator(); }

	void AddNotifyReciver(ResourceUpdateNotifier::IChain *nr)     { _updateNotifier.Add(nr); }
	void RemoveNotifyReciver(ResourceUpdateNotifier::IChain *nr) { _updateNotifier.Remove(nr); }
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
template <typename T, U32 ResTypeId, MemoryAllocator Alloc = Allocators::default, bool isLoadable = true>
class ResoureImpl : public ResourceImplementationInterface, public Allocators::Ext<Alloc>
{
public:
	class ResFactory : public ResourceFactoryChain, public Allocators::Ext<Alloc>
	{
	public:
		ResourceImplementationInterface *Create(ResourceBase *res) { return make_new<T>(res); }
		void Destroy(void *ptr) { make_delete<T>(ptr); }
		IMemoryAllocator *GetMemoryAllocator() { return Alloc; }
		ResFactory() : ResourceFactoryChain()  { TypeId = ResTypeId; }
		bool IsLoadable() { return isLoadable; }
	};
	ResourceFactoryInterface *GetFactory() { return &_resourceFactory; }

	static ResFactory _resourceFactory;
	static U32 GetTypeId() { return _resourceFactory.TypeId; }
};

template <typename T, U32 ResTypeId, MemoryAllocator Alloc, bool IsLoadable>
typename ResoureImpl<T, ResTypeId, Alloc, IsLoadable>::ResFactory ResoureImpl<T, ResTypeId, Alloc, IsLoadable>::_resourceFactory;


// ============================================================================

class BAMS_EXPORT ResourceManager : public Allocators::Ext<>
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
	ResourceBase *GetByFilename(const WSTR &filename, U32 typeId = RESID_UNKNOWN);
	ResourceBase *Get(const STR &resName, U32 typeId = RESID_UNKNOWN);
	ResourceBase *Get(CSTR resName, U32 typeId = RESID_UNKNOWN) { return Get(STR(resName), typeId); };
	ResourceBase *Get(const UUID &resUID);

	template<class T>
	ResourceBase *Get(const STR &resName) { return Get(resName, T::GetTypeId()); }

	template<class T>
	ResourceBase *Get(CSTR resName) { return Get(resName, T::GetTypeId()); }

	ResourceBase *Add(CWSTR path);
	void AddDir(CWSTR path);
	void RootDir(CWSTR path);

	void LoadSync(ResourceBase *res = nullptr);
	void RefreshResorceFileInfo(ResourceBase *res);

	void StartDirectoryMonitor();
	void StopDirectoryMonitor();
	void AbsolutePath(WSTR &filename, const WSTR *_root = nullptr);
	void RelativePath(WSTR &filename, const WSTR *_root = nullptr);
	WSTR GetDirPath(const WString &filename);
};


// ============================================================================

class ResourceManagerModule : public IModule
{
public:
	void Update(float dt);
	void Initialize();
	void Finalize();
	void SendMsg(Message *msg);
	~ResourceManagerModule();

	U32 GetModuleId() { return IModule::ResourceManagerModule; }

	ResourceManager *GetResourceManager();
};

extern ResourceManager *globalResourceManager;

#define RESOURCEMANAGER_ADD_FILE 0x10001

#include "RawData.h"
#include "ResShader.h"
#include "ResMesh.h"
