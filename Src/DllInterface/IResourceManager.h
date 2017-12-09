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

	BAMS_EXPORT IResourceManager *Create_ResourceManager();
	BAMS_EXPORT void Destroy_ResourceManager(IResourceManager *);

	class ResourceManager {
		IResourceManager *_rm;

	public:
		ResourceManager() { _rm = Create_ResourceManager(); }
		ResourceManager(IResourceManager *rm) : _rm(rm) {};
		~ResourceManager() { Destroy_ResourceManager(_rm); _rm = nullptr; }
	};

	//BAMS_EXPORT
	
	BAMS_EXPORT void IResourceManager_AddResource(const char *path);
	BAMS_EXPORT IResource *IResourceManager_FindByName(const char *name);
	BAMS_EXPORT IResource *IResourceManager_FindByUID(const UUID &uid);
	BAMS_EXPORT UUID IResourceManager_GetUID(IResource *res);
	BAMS_EXPORT const char * IResourceManager_GetName(IResource *res);
	BAMS_EXPORT const char * IResourceManager_GetPath(IResource *res);
	BAMS_EXPORT bool IResourceManager_IsLoaded(IResource *res);

	class Resource {
		IResource *_res;
	public:
		Resource(IResource *r) : _res(r) {}

		UUID GetUID() { return IResourceManager_GetUID(_res); }
		const char *GetName() { return IResourceManager_GetName(_res); }
		const char *GetPath() { return IResourceManager_GetPath(_res); }
		bool IsLoaded() { return IResourceManager_IsLoaded(_res); }

	};

	BAMS_EXPORT IRawData IResourceManager_GetByUID_RawData(const UUID &uid);
	BAMS_EXPORT IRawData IResourceManager_GetByName_RawData(const char *name);
}