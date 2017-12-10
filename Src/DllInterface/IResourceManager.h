/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "DllInterface\DllInterface.h"
*
*/

extern "C" {
	typedef void IResource;
	typedef void IRawData;
	typedef void IResourceManager;

	// Resource
	BAMS_EXPORT UUID IResource_GetUID(IResource *res);
	BAMS_EXPORT const char * IResource_GetName(IResource *res);
	BAMS_EXPORT const char * IResource_GetPath(IResource *res);
	BAMS_EXPORT bool IResource_IsLoaded(IResource *res);

	// ResourceManager
	BAMS_EXPORT IResourceManager *IResourceManager_Create();
	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *);
	BAMS_EXPORT void IResourceManager_AddResource(IResourceManager *rm, const char *path);
	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name);
	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID &uid);
	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name);


	

	class Resource 
	{
		IResource *_res;

	public:
		Resource(IResource *r) : _res(r) {}

		UUID GetUID() { return IResource_GetUID(_res); }
		const char *GetName() { return IResource_GetName(_res); }
		const char *GetPath() { return IResource_GetPath(_res); }
		bool IsLoaded() { return IResource_IsLoaded(_res); }
	};

	class RawData
	{
		IRawData *_res;

	public:
		RawData(IRawData *r) : _res(r) {}

		UUID GetUID() { return IResource_GetUID(static_cast<IResource*>(_res)); }
		const char *GetName() { return IResource_GetName(static_cast<IResource*>(_res)); }
		const char *GetPath() { return IResource_GetPath(static_cast<IResource*>(_res)); }
		bool IsLoaded() { return IResource_IsLoaded(static_cast<IResource*>(_res)); }
	};

	class ResourceManager 
	{
		IResourceManager *_rm;

	public:
		ResourceManager() { _rm = IResourceManager_Create(); }
		ResourceManager(IResourceManager *rm) : _rm(rm) {};
		~ResourceManager() { IResourceManager_Destroy(_rm); _rm = nullptr; }

		void AddResource(const char *path) { IResourceManager_AddResource(_rm, path); }

		Resource FindByName(const char *name) { Resource res(IResourceManager_FindByName(_rm, name)); return res; }
		Resource FindByUID(const UUID &uid) { Resource res(IResourceManager_FindByUID(_rm, uid)); return res; }

		RawData GetRawDataByName(const char *name) { RawData res(IResourceManager_GetRawDataByName(_rm, name)); return res; }
		RawData GetRawDataByUID(const UUID &uid) { RawData res(IResourceManager_GetRawDataByUID(_rm, uid)); return res; }

	};
}