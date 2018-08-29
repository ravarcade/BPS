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
	BAMS_EXPORT UUID IResource_GetUID(IResource *res);
	BAMS_EXPORT const char * IResource_GetName(IResource *res);
	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res);
	BAMS_EXPORT bool IResource_IsLoaded(IResource *res);
	BAMS_EXPORT uint32_t IResource_GetType(IResource *res);
	BAMS_EXPORT void IResource_Release(IResource *res);

	// ResourceManager
	BAMS_EXPORT IResourceManager *IResourceManager_Create();
	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT IResource *IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT void IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path);
	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void IResourceManager_Filter(IResourceManager *rm, IResource **resList, uint32_t *resCount, const char *pattern);
	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_LoadAsync(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_StartDirectoryMonitor(IResourceManager *rm);
	BAMS_EXPORT void IResourceManager_StopDirectoryMonitor(IResourceManager *rm);

	// RawData
	BAMS_EXPORT unsigned char *IRawData_GetData(IRawData *res);
	BAMS_EXPORT size_t IRawData_GetSize(IRawData *res);

	class CResource 
	{
		IResource *_res;

	public:
		CResource(IResource *r) : _res(r) {}
		~CResource() { IResource_Release(_res); }

		UUID GetUID() { return IResource_GetUID(_res); }
		const char *GetName() { return IResource_GetName(_res); }
		const wchar_t *GetPath() { return IResource_GetPath(_res); }
		bool IsLoaded() { return IResource_IsLoaded(_res); }
		uint32_t GetType() { return IResource_GetType(static_cast<IResource*>(_res)); }
	};

	class CRawData
	{
		IRawData *_res;

	public:
		CRawData(IRawData *r) : _res(r) {}
		~CRawData() { IResource_Release(static_cast<IResource*>(_res)); }

		UUID GetUID() { return IResource_GetUID(static_cast<IResource*>(_res)); }
		const char *GetName() { return IResource_GetName(static_cast<IResource*>(_res)); }
		const wchar_t *GetPath() { return IResource_GetPath(static_cast<IResource*>(_res)); }
		bool IsLoaded() { return IResource_IsLoaded(static_cast<IResource*>(_res)); }
		uint32_t GetType() { return IResource_GetType(static_cast<IResource*>(_res)); }
		unsigned char *GetData() { return IRawData_GetData(_res); }
		size_t GetSize() { return IRawData_GetSize(_res); }
	};

	class CResourceManager 
	{
		IResourceManager *_rm;

	public:
		CResourceManager() { _rm = IResourceManager_Create(); }
		CResourceManager(IResourceManager *rm) : _rm(rm) {};
		~CResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

		CResource AddResource(const wchar_t *path) 
		{ 
			CResource res(IResourceManager_AddResource(_rm, path)); 
			return res; }
		void AddDir(const wchar_t *path) { IResourceManager_AddDir(_rm, path); }

		void Filter(IResource **resList, uint32_t *resCount, const char *pattern) { IResourceManager_Filter(_rm, resList, resCount, pattern); }
		CResource FindByName(const char *name) { CResource res(IResourceManager_FindByName(_rm, name)); return res; }
		CResource FindByUID(const UUID &uid) { CResource res(IResourceManager_FindByUID(_rm, uid)); return res; }

		CRawData GetRawDataByName(const char *name) { CRawData res(IResourceManager_GetRawDataByName(_rm, name)); return res; }
		CRawData GetRawDataByUID(const UUID &uid) { CRawData res(IResourceManager_GetRawDataByUID(_rm, uid)); return res; }

		void LoadSync() { IResourceManager_LoadSync(_rm); }
		void LoadAsync() { IResourceManager_LoadAsync(_rm); }

		void StartDirectoryMonitor() { IResourceManager_StartDirectoryMonitor(_rm); }
		void StopDirectoryMonitor() { IResourceManager_StopDirectoryMonitor(_rm); }

	};


	BAMS_EXPORT void DoTests();
}