/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class IResourceFactory;

/// <summary>
/// Interface to resource implementation
/// </summary>
class IResImp
{
public:
	virtual ~IResImp() {}
	virtual void Update(ResBase *) = 0;
	virtual void Release(ResBase *) = 0;
	virtual IResourceFactory *GetFactory() = 0;
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
class IResourceFactory
{
public:
	U32 TypeId;
	virtual ~IResourceFactory() {}
	virtual IResImp *Create(ResBase *res) = 0;
	virtual void Destroy(void *ptr) = 0;
	virtual IMemoryAllocator *GetMemoryAllocator() = 0;
	virtual bool IsLoadable() = 0;
};

class ResourceFactoryChain : public IResourceFactory
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

// ============================================================================ ResImp ===

/// <summary>
/// Helper class used to make resource implementation easier.
/// It will add ResFactory to class.
/// </summary>
template <typename T, U32 ResTypeId, bool isLoadable = true, MemoryAllocator Alloc = Allocators::default>
class ResImp : public IResImp, public Allocators::Ext<Alloc>
{
public:
	class ResFactory : public ResourceFactoryChain, public Allocators::Ext<Alloc>
	{
	public:
		IResImp *Create(ResBase *res) { return make_new<T>(res); }
		void Destroy(void *ptr) { make_delete<T>(ptr); }
		IMemoryAllocator *GetMemoryAllocator() { return Alloc; }
		ResFactory() : ResourceFactoryChain() { TypeId = ResTypeId; }
		bool IsLoadable() { return isLoadable; }
	};
	IResourceFactory *GetFactory() { return &_resourceFactory; }

	static ResFactory _resourceFactory;
	static U32 GetTypeId() { return _resourceFactory.TypeId; }
};

template <typename T, U32 ResTypeId, bool IsLoadable, MemoryAllocator Alloc>
typename ResImp<T, ResTypeId, IsLoadable, Alloc>::ResFactory ResImp<T, ResTypeId, IsLoadable, Alloc>::_resourceFactory;

// ============================================================================ ResPure ===

/// <summary>
/// Helper class used to make resource implementation easier.
/// It will add ResFactory to class.
/// </summary>
template <typename T, U32 ResTypeId, MemoryAllocator Alloc = Allocators::default>
class ResPure : public IResImp, public Allocators::Ext<Alloc>
{
public:
	class ResFactory : public ResourceFactoryChain, public Allocators::Ext<Alloc>
	{
	public:
		IResImp *Create(ResBase *res) { return make_new<T>(res); }
		void Destroy(void *ptr) { make_delete<T>(ptr); }
		IMemoryAllocator *GetMemoryAllocator() { return Alloc; }
		ResFactory() : ResourceFactoryChain() { TypeId = ResTypeId; }
		bool IsLoadable() { return false; }
	};
	IResourceFactory *GetFactory() { return &_resourceFactory; }

	static ResFactory _resourceFactory;
	static U32 GetTypeId() { return _resourceFactory.TypeId; }
};

template <typename T, U32 ResTypeId,MemoryAllocator Alloc>
typename ResPure<T, ResTypeId, Alloc>::ResFactory ResPure<T, ResTypeId, Alloc>::_resourceFactory;