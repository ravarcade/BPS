#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

#include "DllInterface.h"

NAMESPACE_BAMS_BEGIN
extern "C" {

	BAMS_EXPORT UUID IResource_GetUID(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->UID;
	}

	BAMS_EXPORT const char * IResource_GetName(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->Name;
	}

	BAMS_EXPORT const char * IResource_GetPath(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->Path;
	}

	BAMS_EXPORT bool IResource_IsLoaded(IResource *res)
	{
		CORE::ResourceBase *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb->isLoaded();
	}

	// ===========================================================================

	BAMS_EXPORT IResourceManager *IResourceManager_Create()
	{
		return BAMS::CORE::ResourceManager::Create();
	}

	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *rm)
	{
		BAMS::CORE::ResourceManager::Destroy(reinterpret_cast<CORE::ResourceManager *>(rm));
	}

	BAMS_EXPORT void IResourceManager_AddResource(IResourceManager *rm, const char *path)
	{
		CORE::ResourceManager *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		resm->Add(path);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name)
	{
		CORE::ResourceManager *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Find(name);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid)
	{
		CORE::ResourceManager *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Find(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID & uid)
	{
		CORE::ResourceManager *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Get<CORE::RawData>(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GeRawDatatByName(IResourceManager *rm, const char * name)
	{
		CORE::ResourceManager *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Get<CORE::RawData>(name);
	}

}
NAMESPACE_BAMS_END