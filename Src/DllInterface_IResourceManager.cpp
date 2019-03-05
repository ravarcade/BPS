#include "stdafx.h"
#include "DllInterface.h"

NAMESPACE_BAMS_BEGIN
using namespace CORE;

extern "C" {


	BAMS_EXPORT void Initialize()
	{
		setlocale(LC_CTYPE, "");
		RegisteredClasses::Initialize();
		IEngine::Initialize();
	}

	BAMS_EXPORT void Finalize()
	{
		IEngine::Finalize();
		RegisteredClasses::Finalize();
	}

	BAMS_EXPORT BAMS::IModule *GetModule(uint32_t moduleId)
	{
		return IEngine::GetModule(moduleId);
	}
	// =========================================================================== Resource

	BAMS_EXPORT void IResource_AddRef(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		rb->AddRef();
	}

	BAMS_EXPORT void IResource_Release(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		rb->Release();
	}

	BAMS_EXPORT uint32_t IResource_GetRefCounter(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb->GetRefCounter();
	}

	BAMS_EXPORT UUID IResource_GetUID(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->UID : Tools::NOUID;
	}

	BAMS_EXPORT const char * IResource_GetName(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->Name.c_str() : "";
	}

	BAMS_EXPORT const wchar_t * IResource_GetPath(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->Path.c_str() : L"";
	}

	BAMS_EXPORT bool IResource_IsLoaded(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->isLoaded() : false;
	}

	BAMS_EXPORT uint32_t IResource_GetType(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->Type : false;
	}

	// =========================================================================== RawData

	BAMS_EXPORT unsigned char * IRawData_GetData(IRawData *res)
	{
		auto *rb = reinterpret_cast<RawData *>( reinterpret_cast<ResourceBase *>(res)->GetImplementation() );
		return static_cast<unsigned char *>(rb->GetData());
	}

	BAMS_EXPORT size_t IRawData_GetSize(IRawData *res)
	{
		auto *rb = reinterpret_cast<RawData *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return rb->GetSize();
	}

	// =========================================================================== Shader

	BAMS_EXPORT unsigned char * IShader_GetData(IShader *res)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return static_cast<unsigned char *>(rb->GetData());
	}

	BAMS_EXPORT size_t IShader_GetSize(IShader *res)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return rb->GetSize();
	}

	BAMS_EXPORT void IShader_AddProgram(IShader *res, const wchar_t *fileName)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		rb->AddProgram(fileName);
	}

	BAMS_EXPORT const wchar_t * IShader_GetProgramName(IShader *res, int type)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return rb->GetSubprogramName(type).c_str();
	}

	BAMS_EXPORT void IShader_Save(IShader *res)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		rb->Save(reinterpret_cast<ResourceBase *>(res));
	}

	BAMS_EXPORT uint32_t IShader_GetSubprogramsCount(IShader *res)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return rb->GetSubprogramsCount();
	}

	BAMS_EXPORT IRawData *IShader_GetSubprogram(IShader *res, uint32_t idx)
	{
		auto *rb = reinterpret_cast<ResShader *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
		return rb->GetSubprogram(idx);
	}

	// =========================================================================== ResourceManager

	BAMS_EXPORT IResourceManager *IResourceManager_Create()
	{
		auto rm = reinterpret_cast<ResourceManagerModule*>(GetModule(IModule::ResourceManagerModule));
		return rm->GetResourceManager();
//		return BAMS::ResourceManager::Create();
	}

	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *rm)
	{
		// do nothing.... ResourceManage is global object.
//		BAMS::ResourceManager::Destroy(reinterpret_cast<ResourceManager *>(rm));
	}

	BAMS_EXPORT IResource *IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Add(path);
	}

	BAMS_EXPORT void IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->AddDir(path);
	}

	BAMS_EXPORT void IResourceManager_RootDir(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->RootDir(path);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name, uint32_t type)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(name, type);
	}

	BAMS_EXPORT void IResourceManager_Filter(IResourceManager *rm, IResource ** resList, uint32_t * resCount, const char * pattern)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Filter(reinterpret_cast<ResourceBase **>(resList), resCount, pattern);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID & uid)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(uid);
	}

	BAMS_EXPORT IRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get<RawData>(name);
	}

	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm, IResource *res)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->LoadSync(reinterpret_cast<ResourceBase *>(res));
	}

	BAMS_EXPORT void IResourceManager_RefreshResorceFileInfo(IResourceManager *rm, IResource *res)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->RefreshResorceFileInfo(reinterpret_cast<ResourceBase *>(res));
	}

	BAMS_EXPORT void IResourceManager_StartDirectoryMonitor(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->StartDirectoryMonitor();
	}

	BAMS_EXPORT void IResourceManager_StopDirectoryMonitor(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->StopDirectoryMonitor();
	}


	// ========================================================================

	BAMS_EXPORT void GetMemoryAllocationStats(uint64_t *Max, uint64_t *Current, uint64_t *Counter)
	{
		Allocators::GetMemoryAllocationStats(Max, Current, Counter);
	}

	BAMS_EXPORT bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data)
	{
		return Allocators::GetMemoryBlocks(current, size, counter, data);
	}

	// ========================================================================


	BAMS_EXPORT void IEngine_RegisterExternalModule(
		uint32_t moduleId,
		ExternalModule_Initialize *moduleInitialize,
		ExternalModule_Finalize *moduleFinalize,
		ExternalModule_Update *moduleUpdate,
		BAMS::ExternalModule_SendMsg *moduleSendMsg,
		ExternalModule_Destroy *moduleDestroy,
		const void *moduleData)
	{
		BAMS::CORE::IEngine::RegisterExternalModule(moduleId, moduleInitialize, moduleFinalize, moduleUpdate, reinterpret_cast<CORE::IEngine::ExternalModule_SendMsg*>(moduleSendMsg), moduleDestroy, moduleData);
	}
	
	BAMS_EXPORT void IEngine_SendMsg(
		uint32_t msgId,
		uint32_t msgDst,
		uint32_t msgSrc,
		const void *data)
	{
		CORE::Message msg = { msgId, msgDst, msgSrc, data };
		BAMS::CORE::IEngine::SendMsg(&msg);
	}

	BAMS_EXPORT void IEngine_Update(float dt)
	{
		BAMS::CORE::IEngine::Update(dt);
	}

	BAMS_EXPORT BAMS::CORE::Allocators::IMemoryAllocator * GetMemoryAllocator(uint32_t allocatorType, SIZE_T size)
	{
		return BAMS::CORE::Allocators::GetMemoryAllocator(allocatorType, size);
	}
	
	// ========================================================================

	void TestRingBuffer()
	{
		auto &_alloc = *Allocators::GetMemoryAllocator(IMemoryAllocator::block, 16*1024);

		auto a1 = _alloc.allocate(4000);
		auto a2 = _alloc.allocate(4000);
		auto a3 = _alloc.allocate(4000);
		auto a4 = _alloc.allocate(4000);
		_alloc.deallocate(a1);
		auto a5 = _alloc.allocate(4000);
		_alloc.deallocate(a2);
		auto a6 = _alloc.allocate(4000);
		_alloc.deallocate(a3);
		auto a7 = _alloc.allocate(4000);
		_alloc.deallocate(a7);
		_alloc.deallocate(a5);
		_alloc.deallocate(a4);
		auto a8 = _alloc.allocate(4000);

	}

	void testb(const STR &s)
	{
		printf("[%s]", s.c_str());
	}

	BAMS_EXPORT void DoTests()
	{
		return;
		{
			WSTR t;
			int ll = t.size();
			TRACE(ll << "\n");
			TRACEW(t.c_str());
			TRACE(((int)t.size()) << "\n");
		}
		return;
		STR test = "aaabb";
		test += 123;
		printf("str123=%s\n", test.c_str());
		CStringHastable<int> hti;
		hti.find("hello");
		*hti.add("hello") = 5;
		printf("[%d]", hti["hello"]);
		hti["hello"] = 7;
		printf("[%d]", hti["hello"]);

		struct st {
			int a;
			int b;
			int c;
			st() : a(0), b(1), c(2) {}
		};
		CStringHastable<st> hts;
		printf("[%llx]", (uint64_t) hts.find("hello"));
		st as;
		as.a = 77;
		as.b = 3;
		*hts.add("hello") = as;
		printf("[%llx, %d, %d, %d]", (uint64_t)hts.find("hello"), hts["hello"].a, hts["hello"].b , hts["hello"].c);
		hts["hello"].c = 123;
		printf("[%llx, %d, %d, %d]", (uint64_t)hts.find("hello"), hts["hello"].a, hts["hello"].b, hts["hello"].c);
		hts["hello"] = as;
		printf("[%llx, %d, %d, %d]", (uint64_t)hts.find("hello"), hts["hello"].a, hts["hello"].b, hts["hello"].c);


		struct dn {
			std::string txt;
			int a;
			dn() : txt("hello world"), a(1) {}
		};

		CStringHastable<dn> htd;
		printf("[%llx]", (uint64_t)htd.find("hello"));
		dn d;
		d.txt = "ping";
		*htd.add("hello") = d;
		printf("[%llx, %d, %s]", (uint64_t)htd.find("hello"), htd["hello"].a, htd["hello"].txt.c_str());
		htd["hello"].a = 123;
		printf("[%llx, %d, %s]", (uint64_t)htd.find("hello"), htd["hello"].a, htd["hello"].txt.c_str());
		htd["hello"] = d;
		htd["hello"].txt = "boing";
		printf("[%llx, %d, %s]", (uint64_t)htd.find("hello"), htd["hello"].a, htd["hello"].txt.c_str());

		return;
		hashtable<STR, int> ht;
		STR kkey("hahaha");
		ht[kkey] = 11;
		ht[kkey] = 22;
		ht["hahaha"] = 33;
		int zz = ht[kkey];
		testb("TESTB");
		return;
		TestRingBuffer();
		{
			shared_string<char> s1;
			shared_string<char> s2("hello");
			shared_string<char> s3(s2);
		}


//		return;
		// basic_array tests
		basic_array<U32> ba1;
		basic_array<U32> ba2(10);
		basic_array<U32> ba3({ 5,4,3,2,1 });
		for each (auto v in ba3)
		{
			ba1.push_back(v);
		}
		ba2 = ba1;
		ba2 += ba3;

//		return;
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

		basic_string<wchar_t, U16> sw1 = L"[sw1]";
		WSTR w8(sw1);
		w1 = w7 + sw1;
		sw1 += w1;
		const wchar_t *w = sw1.c_str();

		WSTR w9(t1);
		STR t9(w1);

		WSTR w10 = L"��w ninja.";
		STR t10 = w10;
		STR t12 = setlocale(LC_ALL, NULL);
		setlocale(LC_CTYPE, "");
		STR t13 = setlocale(LC_ALL, NULL);
		STR t11 = "��w ninja.";
		WSTR w11 = t11;
		wprintf(w11.c_str());
	}
}
NAMESPACE_BAMS_END