#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

#include "DllInterface.h"

NAMESPACE_BAMS_BEGIN
using namespace CORE;

struct MonitoredDirEvent
{
	U32 action;
	PathSTR path;
	WSimpleString fileName;
	WSimpleString fileNameRenameOld;

	MonitoredDirEvent(IMemoryAllocator *alloc, MonitoredDirEvent &src) :
		action(src.action),
		path(src.path),
		fileName(alloc, src.fileName),
		fileNameRenameOld(alloc, src.fileNameRenameOld)
	{}

	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const WSimpleString::_B &_path, const WSimpleString::_B &_fn) :
		action(_action),
		path(_path),
		fileName(alloc, _fn),
		fileNameRenameOld()
	{}

	template<typename T>
	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, FILE_NOTIFY_INFORMATION* pNotify, T &_fileNameRenameOld) :
		action(_action),
		path(_path),
		fileName(alloc, pNotify->FileName, reinterpret_cast<WCHAR *>(reinterpret_cast<U8 *>(pNotify->FileName) + pNotify->FileNameLength)),
		fileNameRenameOld(alloc, _fileNameRenameOld)
	{}

	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, FILE_NOTIFY_INFORMATION* pNotify) :
		action(_action),
		path(_path),
		fileName(alloc, pNotify->FileName, reinterpret_cast<WCHAR *>(reinterpret_cast<U8 *>(pNotify->FileName) + pNotify->FileNameLength)),
		fileNameRenameOld()
	{}
};

extern "C" {


	BAMS_EXPORT void Initialize()
	{
		setlocale(LC_CTYPE, "");
		RegisteredClasses::Initialize();
	}

	BAMS_EXPORT void Finalize()
	{
		RegisteredClasses::Finalize();
	}

	// =========================================================================== Resource

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

	BAMS_EXPORT uint32_t IResource_GetType(IResource * res)
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

	// =========================================================================== ResourceManager

	BAMS_EXPORT IResourceManager *IResourceManager_Create()
	{
		return BAMS::ResourceManager::Create();
	}

	BAMS_EXPORT void IResourceManager_Destroy(IResourceManager *rm)
	{
		BAMS::ResourceManager::Destroy(reinterpret_cast<ResourceManager *>(rm));
	}

	BAMS_EXPORT void IResourceManager_AddResource(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->Add(path);
	}

	BAMS_EXPORT void IResourceManager_AddDir(IResourceManager *rm, const wchar_t *path)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->AddDir(path);
	}

	BAMS_EXPORT IResource *IResourceManager_FindByName(IResourceManager *rm, const char *name)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		return resm->Get(name);
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

	BAMS_EXPORT void IResourceManager_LoadSync(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->LoadSync();
	}

	BAMS_EXPORT void IResourceManager_LoadAsync(IResourceManager *rm)
	{
		auto *resm = reinterpret_cast<ResourceManager *>(rm);
		resm->LoadAsync();
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
		if (Max)
			*Max = Allocators::Default::MaxAllocatedMemory;

		if (Current)
			*Current = Allocators::Default::CurrentAllocatedMemory;

		if (Counter)
			*Counter = Allocators::Default::TotalAllocateCommands;
	}

	BAMS_EXPORT bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data)
	{
		return Allocators::Debug::list(current, size, counter, data);
	}

	// ========================================================================


	void TestRingBuffer()
	{
		
		Allocators::RingBuffer _alloc(MemoryAllocatorGlobal<>::GetMemoryAllocator(), 8192);

		auto a1 = _alloc.allocate(2000);
		auto a2 = _alloc.allocate(2000);
		auto a3 = _alloc.allocate(2000);
		auto a4 = _alloc.allocate(2000);
		_alloc.deallocate(a1);
		auto a5 = _alloc.allocate(2000);
		_alloc.deallocate(a2);
		auto a6 = _alloc.allocate(2000);
		_alloc.deallocate(a3);
		auto a7 = _alloc.allocate(2000);
		_alloc.deallocate(a7);
		_alloc.deallocate(a5);
		_alloc.deallocate(a4);
		auto a8 = _alloc.allocate(2000);
		
		base_ringbuffer<MonitoredDirEvent> events(8192);

		wchar_t test[] = L"abcdefghijklmnopq";
		PathSTR path = L"c:\\cos\\cs";
		PathSTR fn = L"nazwaplik.txt";

		events.push_back(100, path, fn);
		events.push_back(101, path, fn);
		events.push_back(102, path, fn);
		events.resize(16384);
		events.push_back(200, path, fn);
		events.push_back(201, path, fn);
		events.push_back(202, path, fn);
		events.resize(32768);

		while (!events.empty())
		{
			auto ev = events.front();
			TRACE("ev: " << ev->action << "\n");
			events.pop();
		}


	}

	BAMS_EXPORT void DoTests()
	{
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
//		STR t8(st1);
		STR t8(st1.ToBasicString());
		t1 = t7 + st1.ToBasicString();
		st1 += t1.ToBasicString();
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
		sw1 += w1.ToBasicString();
		const wchar_t *w = sw1.c_str();

		WSTR w9(t1.ToBasicString());
		STR t9(w1.ToBasicString());

		WSTR w10 = L"¯ó³w ninja.";
		STR t10 = w10.ToBasicString();
		STR t12 = setlocale(LC_ALL, NULL);
		setlocale(LC_CTYPE, "");
		STR t13 = setlocale(LC_ALL, NULL);
		STR t11 = "¯ó³w ninja.";
		WSTR w11 = t11.ToBasicString();
	}
}
NAMESPACE_BAMS_END