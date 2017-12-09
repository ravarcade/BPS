#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

#include "DllInterface.h"

BAMS::CORE::ResourceManager rm;

NAMESPACE_BAMS_BEGIN
extern "C" {

	BAMS_EXPORT IResourceManager *Create_ResourceManager()
	{
		return BAMS::CORE::ResourceManager::Create();
	}

	BAMS_EXPORT void Destroy_ResourceManager(IResourceManager *rm)
	{
		BAMS::CORE::ResourceManager::Destroy(reinterpret_cast<CORE::ResourceManager *>(rm));
	}

	BAMS_EXPORT void IResourceManager_AddResource(const char *path)
	{
		rm.Add(path);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByName(const char *name)
	{
		return rm.Find(name);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByUID(const UUID &uid)
	{
		return rm.Find(uid);
	}

	BAMS_EXPORT UUID IResourceManager_GetUID(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->UID;
	}

	BAMS_EXPORT const char * IResourceManager_GetName(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->Name;
	}

	BAMS_EXPORT const char * IResourceManager_GetPath(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->Path;
	}

	BAMS_EXPORT bool IResourceManager_IsLoaded(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->isLoaded();
	}

	BAMS_EXPORT IRawData *IResourceManager_GetByUID_RawData(const UUID & uid)
	{
		return rm.Get<CORE::RawData>(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetByName_RawData(const char * name)
	{
		return rm.Get<CORE::RawData>(name);
	}

}
NAMESPACE_BAMS_END