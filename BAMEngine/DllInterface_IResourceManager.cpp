#include "stdafx.h"

namespace BAMS {

using namespace CORE;

template<typename T>
T *Impl(void *res)
{
	assert(res);
	auto ret = reinterpret_cast<T *>(reinterpret_cast<ResourceBase *>(res)->GetImplementation());
	assert(ret);
	return ret;
}

extern "C" {
	static STR *pRetStrHelper;

	BAMS_EXPORT void Initialize()
	{
		setlocale(LC_CTYPE, "");
		RegisteredClasses::Initialize();
		IEngine::Initialize();
		pRetStrHelper = new STR();
	}

	BAMS_EXPORT void Finalize()
	{
		delete pRetStrHelper;
		IEngine::Finalize();
		RegisteredClasses::Finalize();
	}

	BAMS_EXPORT BAMS::IModule *GetModule(uint32_t moduleId)
	{
		return IEngine::GetModule(moduleId);
	}

	// =========================================================================== Resource

	BAMS_EXPORT void IResource_AddRef(IResource *res) { assert(res); reinterpret_cast<ResourceBase *>(res)->AddRef(); }
	BAMS_EXPORT void IResource_Release(IResource *res) { assert(res); reinterpret_cast<ResourceBase *>(res)->Release(); }
	BAMS_EXPORT uint32_t IResource_GetRefCounter(IResource *res) { assert(res); return reinterpret_cast<ResourceBase *>(res)->GetRefCounter(); }
	BAMS_EXPORT UUID IResource_GetUID(IResource *res) { assert(res); return reinterpret_cast<ResourceBase *>(res)->UID; }
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

	BAMS_EXPORT bool IResource_IsLoadable(IResource *res)
	{
		auto *rb = reinterpret_cast<ResourceBase *>(res);
		return rb ? rb->isLoadable() : false;
	}

	// =========================================================================== ResRawData

	BAMS_EXPORT uint8_t * IResRawData_GetData(IResRawData *res) { return Impl<ResRawData>(res)->GetData(); }
	BAMS_EXPORT size_t IResRawData_GetSize(IResRawData *res   ) { return Impl<ResRawData>(res)->GetSize(); }

	// =========================================================================== ResShader

	BAMS_EXPORT uint8_t * IResShader_GetData(IResShader *res)                           { return Impl<ResShader>(res)->GetData(); }
	BAMS_EXPORT size_t IResShader_GetSize(IResShader *res)                              { return Impl<ResShader>(res)->GetSize(); }
	BAMS_EXPORT void IResShader_AddProgram(IResShader *res, const wchar_t *fileName)    {        Impl<ResShader>(res)->AddProgram(fileName); }
	BAMS_EXPORT const wchar_t * IResShader_GetSourceFilename(IResShader *res, int type) { return Impl<ResShader>(res)->GetSourceFilename(type).c_str(); }
	BAMS_EXPORT const wchar_t * IResShader_GetBinaryFilename(IResShader *res, int type) { return Impl<ResShader>(res)->GetBinaryFilename(type).c_str(); }
	BAMS_EXPORT void IResShader_Save(IResShader *res)                                   {        Impl<ResShader>(res)->Save(reinterpret_cast<ResourceBase *>(res));}
	BAMS_EXPORT uint32_t IResShader_GetBinaryCount(IResShader *res)                     { return Impl<ResShader>(res)->GetBinaryCount(); }
	BAMS_EXPORT IResRawData *IResShader_GetBinary(IResShader *res, uint32_t idx)        { return Impl<ResShader>(res)->GetBinary(idx); }

	// =========================================================================== ResMesh

	BAMS_EXPORT void IResMesh_SetVertexDescription(IResMesh * res, IVertexDescription * vd, uint32_t meshHash, IResource * meshSrc, U32 meshIdx)
	{
		Impl<ResMesh>(res)->SetVertexDescription( 
			reinterpret_cast<VertexDescription *>(vd), 
			meshHash, 
			reinterpret_cast<ResourceBase *>(meshSrc), 
			meshIdx);
	}

	BAMS_EXPORT IVertexDescription * IResMesh_GetVertexDescription(IResMesh * res, bool loadASAP) { return Impl<ResMesh>(res)->GetVertexDescription(loadASAP); }
	BAMS_EXPORT IMProperties * IResMesh_GetMeshProperties(IResMesh * res, bool loadASAP)          { return Impl<ResMesh>(res)->GetMeshProperties(loadASAP); }
	BAMS_EXPORT IResource * IResMesh_GetMeshSrc(IResMesh * res)                                   { return Impl<ResMesh>(res)->GetMeshSrc(); }
	BAMS_EXPORT uint32_t IResMesh_GetMeshIdx(IResMesh * res)                                      { return Impl<ResMesh>(res)->GetMeshIdx(); }
	BAMS_EXPORT void IResMesh_SetMeshIdx(IResMesh * res, uint32_t idx)                            { return Impl<ResMesh>(res)->SetMeshIdx(idx); }
	BAMS_EXPORT uint32_t IResMesh_GetMeshHash(IResMesh * res)                                     { return Impl<ResMesh>(res)->GetMeshHash(); }

	// =========================================================================== ResImage

	BAMS_EXPORT Image *IResImage_GetImage(IResImage *res, bool loadASAP) { return Impl<ResImage>(res)->GetImage(loadASAP); }
	BAMS_EXPORT void IResImage_Updated(IResImage *res)                   {        Impl<ResImage>(res)->Updated(); }
	BAMS_EXPORT uint8_t *IResImage_GetSrcData(IResImage *res)            { return Impl<ResImage>(res)->GetSrcData(); }
	BAMS_EXPORT size_t IResImage_GetSrcSize(IResImage *res)              { return Impl<ResImage>(res)->GetSrcSize(); }
	BAMS_EXPORT void IResImage_ReleaseSrc(IResImage *res)                {        Impl<ResImage>(res)->ReleaseSrc(); }

	// =========================================================================== ResourceManager

	BAMS_EXPORT IResourceManager *IResourceManager_Create()
	{
		auto rm = reinterpret_cast<ResourceManagerModule*>(GetModule(IModule::ResourceManagerModule));
		return rm->GetResourceManager();
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

	BAMS_EXPORT IResource *IResourceManager_AddResourceWithoutFile(IResourceManager *rm, const char *resName, uint32_t type)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Add(resName, type);
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

	BAMS_EXPORT void IResourceManager_Filter(IResourceManager *rm, void(*callback)(IResource *, void *), void *localData, const char * pattern, uint32_t typeId)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Filter(reinterpret_cast<void (*)(ResourceBase *, void *)>(callback), localData, pattern, typeId);
	}

	BAMS_EXPORT void IResourceManager_FilterByFilename(IResourceManager * rm, void(*callback)(IResource *, void *), void * localData, const wchar_t * pattern, const wchar_t * rootPath, bool caseInsesitive)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Filter(reinterpret_cast<void(*)(ResourceBase *, void *)>(callback), localData, pattern, rootPath, caseInsesitive);
	}

	BAMS_EXPORT void IResourceManager_FilterByFilenameUTF8(IResourceManager * rm, void(*callback)(IResource *, void *), void * localData, const char * pattern, const wchar_t * rootPath, bool caseInsesitive)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Filter(reinterpret_cast<void(*)(ResourceBase *, void *)>(callback), localData, pattern, rootPath, caseInsesitive);
	}

	BAMS_EXPORT void IResourceManager_FilterByMeshProperty(IResourceManager * rm, void(*callback)(IResource *, void *), void * localData, BAMS::Property * prop, IResMesh * mesh, bool caseInsesitive)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Filter(reinterpret_cast<void(*)(ResourceBase *, void *)>(callback), localData, prop, Impl<ResMesh>(mesh), caseInsesitive);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByUID(IResourceManager *rm, const UUID &uid)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(uid);
	}

	BAMS_EXPORT IResRawData *IResourceManager_GetRawDataByUID(IResourceManager *rm, const UUID & uid)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(uid);
	}

	BAMS_EXPORT IResRawData *IResourceManager_GetRawDataByName(IResourceManager *rm, const char *name)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get<ResRawData>(name);
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
		void *data,
		uint32_t dataLen)
	{
		Message msg = { msgId, msgDst, msgSrc, data, dataLen };
		BAMS::CORE::IEngine::SendMsg(&msg);
	}

	BAMS_EXPORT void IEngine_PostMsg(
		uint32_t msgId,
		uint32_t msgDst,
		uint32_t msgSrc,
		void *data,
		uint32_t dataLen,
		uint32_t delay)
	{
		Message msg = { msgId, msgDst, msgSrc, data, dataLen };
		BAMS::CORE::IEngine::PostMsg(&msg, delay * 1ms);
	}

	BAMS_EXPORT void IEngine_Update(float dt)
	{
		BAMS::CORE::IEngine::Update(dt);
	}

	BAMS_EXPORT Allocators::IMemoryAllocator * GetMemoryAllocator(uint32_t allocatorType, SIZE_T size)
	{
		return Allocators::GetMemoryAllocator(allocatorType, size);
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
		typedef  basic_string_base<char, U32> QQ;
//		QQ a("hello");

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

		WSTR w10 = L"¯ó³w ninja.";
		STR t10 = w10;
		STR t12 = setlocale(LC_ALL, NULL);
		setlocale(LC_CTYPE, "");
		STR t13 = setlocale(LC_ALL, NULL);
		STR t11 = "¯ó³w ninja.";
		WSTR w11 = t11;
		wprintf(w11.c_str());
	}
}

}; // BAMS namespace

