/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "DllInterface.h"
*
*/

extern "C" {
	typedef void IResource;
	typedef void IRawData;
	typedef void IResourceManager;
	typedef void IModule;

	// Call Initialize befor do anything with resources, game engine, etc. Only some memory allocations.
	BAMS_EXPORT void Initialize();

	// Call Finalize as last thing to release all resources and memory
	BAMS_EXPORT void Finalize();

	BAMS_EXPORT IModule *GetModule(uint32_t moduleId);
//	BAMS_EXPORT void SendMessage();

	// MemoryAllocators
	BAMS_EXPORT void GetMemoryAllocationStats(uint64_t *Max, uint64_t *Current, uint64_t *Counter);
	BAMS_EXPORT bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data);

	// Resource
	BAMS_EXPORT void IResource_AddRef(IResource *res);
	BAMS_EXPORT void IResource_Release(IResource *res);
	BAMS_EXPORT uint32_t IResource_GetRefCounter(IResource *res);
	BAMS_EXPORT UUID IResource_GetUID(IResource *res);
	BAMS_EXPORT const char * IResource_GetName(IResource *res);
	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res);
	BAMS_EXPORT bool IResource_IsLoaded(IResource *res);
	BAMS_EXPORT uint32_t IResource_GetType(IResource *res);

	// ResourceManager
	BAMS_EXPORT IResourceManager *IResourceManager_Create();
	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT IResource *IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void IResourceManager_RootDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void IResourceManager_Filter(IResourceManager *rm, IResource **resList, uint32_t *resCount, const char *pattern);
	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_StartDirectoryMonitor(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_StopDirectoryMonitor(IResourceManager *rm);

	// RawData
	BAMS_EXPORT unsigned char *IRawData_GetData(IRawData *res);
	BAMS_EXPORT size_t IRawData_GetSize(IRawData *res);


	class ResourceSmartPtr
	{
	protected:
		struct ResourceSmartPtrData {
			IResource *ptr;
			uint32_t count;
			ResourceSmartPtrData(IResource *p, uint32_t c = 1) : ptr(p), count(c) {}
		} *val;

		void Release()
		{
			if (val && --val->count == 0)
			{
				IResource_Release(val->ptr);
			}
		}

	public:
		ResourceSmartPtr(IResource *p) { val = new ResourceSmartPtrData(p); }
		ResourceSmartPtr(const ResourceSmartPtr &s) : val(s.val) { ++val->count; }
		ResourceSmartPtr(ResourceSmartPtr &&s) : val(s.val) { s.val = nullptr; }
		~ResourceSmartPtr() {
			Release();
		}

		ResourceSmartPtr & operator = (const ResourceSmartPtr &s) { Release(); val = s.val; ++val->count; }
		ResourceSmartPtr & operator = (ResourceSmartPtr &&s) { val = s.val; s.val = nullptr; }

		inline IResource *Get() { return val ? val->ptr : nullptr; }
	};

	class CResource 
	{
	protected:
		ResourceSmartPtr _res;

		inline IResource *Get() { return _res.Get(); }

	public:
		CResource(IResource *r) : _res(r) { AddRef(); }
		CResource(const CResource &src) : _res(src._res) {}
		CResource(CResource &&src) : _res(std::move(src._res)) {}
		virtual ~CResource() {}

		CResource & operator = (const CResource &&src) { _res = std::move(src._res); return *this; }
		CResource & operator = (CResource &&src) { _res = std::move(src._res); return *this; }

		UUID GetUID() { return IResource_GetUID(Get()); }
		const char *GetName() { return IResource_GetName(Get()); }
		const wchar_t *GetPath() { return IResource_GetPath(Get()); }
		bool IsLoaded() { return IResource_IsLoaded(Get()); }
		uint32_t GetType() { return IResource_GetType(Get()); }

		void AddRef()  { IResource_AddRef(Get()); }
		void Release() { IResource_Release(Get()); }
		uint32_t GetRefCounter() { return IResource_GetRefCounter(Get()); }
	};

	class CRawData : public CResource
	{
	public:
		CRawData(IRawData *r) : CResource(static_cast<IResource*>(r)) {}
		CRawData(const CRawData &src) : CResource(src) {}
		CRawData(CRawData &&src) : CResource(std::move(src)) {}
		virtual ~CRawData() {}

		CRawData & operator = (const CRawData &src) { _res = src._res; return *this; }
		CRawData & operator = (CRawData &&src) { _res = std::move(src._res); return *this; }

		unsigned char *GetData() { return IRawData_GetData(static_cast<IRawData*>(Get())); }
		size_t GetSize() { return IRawData_GetSize(static_cast<IRawData*>(Get())); }
	};

	class CResourceManager 
	{
		IResourceManager *_rm;

	public:
		CResourceManager() { _rm = IResourceManager_Create(); }
		CResourceManager(IResourceManager *rm) : _rm(rm) {};
		~CResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

		void AddResource(const wchar_t *path) 
		{ 
			IResourceManager_AddResource(_rm, path); 
		}
		void AddDir(const wchar_t *path) { IResourceManager_AddDir(_rm, path); }
		void RootDir(const wchar_t *path) { IResourceManager_RootDir(_rm, path); }

		void Filter(IResource **resList, uint32_t *resCount, const char *pattern) { IResourceManager_Filter(_rm, resList, resCount, pattern); }
		CResource FindByName(const char *name) { CResource res(IResourceManager_FindByName(_rm, name)); return std::move(res); }
		CResource FindByUID(const UUID &uid) { CResource res(IResourceManager_FindByUID(_rm, uid)); return std::move(res); }

		CRawData GetRawDataByName(const char *name) { CRawData res(IResourceManager_GetRawDataByName(_rm, name)); return std::move(res); }
		CRawData GetRawDataByUID(const UUID &uid) { CRawData res(IResourceManager_GetRawDataByUID(_rm, uid)); return std::move(res); }

		void LoadSync() { IResourceManager_LoadSync(_rm); }

		void StartDirectoryMonitor() { IResourceManager_StartDirectoryMonitor(_rm); }
		void StopDirectoryMonitor() { IResourceManager_StopDirectoryMonitor(_rm); }

	};


	BAMS_EXPORT void DoTests();
}