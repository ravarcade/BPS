#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

#include "DllInterface.h"

NAMESPACE_BAMS_BEGIN
extern "C" {

	BAMS_EXPORT void Init()
	{
		setlocale(LC_CTYPE, "");
	}

	// =========================================================================== Resource

	BAMS_EXPORT UUID IResource_GetUID(IResource *res)
	{
		auto *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb ? rb->UID : CORE::Tools::NOUID;
	}

	BAMS_EXPORT const char * IResource_GetName(IResource *res)
	{
		auto *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb ? rb->Name.c_str() : "";
	}

	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res)
	{
		auto *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb ? rb->Path.c_str() : L"";
	}

	BAMS_EXPORT bool IResource_IsLoaded(IResource *res)
	{
		auto *rb = reinterpret_cast<CORE::ResourceBase *>(res);
		return rb ? rb->isLoaded() : false;
	}

	// =========================================================================== RawData

	BAMS_EXPORT unsigned char * IRawData_GetData(IRawData *res)
	{
		auto *rb = reinterpret_cast<CORE::Resource<CORE::RawData> *>(res)->GetResource();
		return static_cast<unsigned char *>(rb->GetData());
	}

	BAMS_EXPORT size_t IRawData_GetSize(IRawData *res)
	{
		auto *rb = reinterpret_cast<CORE::Resource<CORE::RawData> *>(res)->GetResource();
		return rb->GetSize();
	}

	// =========================================================================== ResourceManager

	BAMS_EXPORT IResourceManager *IResourceManager_Create()
	{
		return BAMS::CORE::ResourceManager::Create();
	}

	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *rm)
	{
		BAMS::CORE::ResourceManager::Destroy(reinterpret_cast<CORE::ResourceManager *>(rm));
	}

	BAMS_EXPORT void IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		resm->Add(path);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Find(name);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Find(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID & uid)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Get<CORE::RawData>(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		return resm->Get<CORE::RawData>(name);
	}

	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		resm->LoadSync();
	}

	BAMS_EXPORT void IResourceManager_LoadAsync(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<CORE::ResourceManager *>(rm);
		resm->LoadAsync();
	}

	// ========================================================================

	BAMS_EXPORT void DoTests()
	{
		return;
		using namespace CORE;
		const char *txtTest = "[hello]";
		STR t1;
		STR t2("[t2]");
		STR t3(txtTest, txtTest + strlen(txtTest));
		STR t4(std::move(t2)); // t2 is empty
		STR t5 = t4;
		STR t6(t5);
		STR t7 = "[t7]";

		ShortSTR st1 = "aaa";
		STR t8(st1);
		t1 = t7 + st1;
		st1 += t1;
		const char *t = st1.c_str();

		const wchar_t *wtxtTest = L"[world]";
		WSTR w1;
		WSTR w2(L"[w2]");
		WSTR w3(wtxtTest, wtxtTest + wcslen(wtxtTest));
		WSTR w4(std::move(w2));
		WSTR w5 = w4;
		WSTR w6(w5);
		WSTR w7 = L"[w7]";

		CORE::basic_string<wchar_t, U16> sw1 = L"[sw1]";
		WSTR w8(sw1);
		w1 = w7 + sw1;
		sw1 += w1;
		const wchar_t *w = sw1.c_str();

		WSTR w9(t1);
		STR t9(w1);

		WSTR w10 = L"¯ó³w ninja.";
		STR t10 = w10;
		STR t12 = setlocale(LC_ALL, NULL);
		setlocale(LC_CTYPE, "");
		STR t13 = setlocale(LC_ALL, NULL);
		STR t11 = "¯ó³w ninja.";
		WSTR w11 = t11;
	}
}
NAMESPACE_BAMS_END