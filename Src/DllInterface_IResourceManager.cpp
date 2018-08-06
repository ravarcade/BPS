#include "stdafx.h"
#include "DllInterface.h"

NAMESPACE_BAMS_BEGIN
using namespace CORE;

struct DiskFileName
{
	DiskFileName() = delete;
	DiskFileName(const DiskFileName &) = delete;
	DiskFileName(const DiskFileName &&) = delete;

	DiskFileName(IMemoryAllocator *alloc, const DiskFileName &src) :
		path(src.path),
		hash(src.hash),
		fileName(alloc, src.fileName)
	{} // we still have to allocate fileName string

	DiskFileName(IMemoryAllocator *alloc, const PathSTR &_path, U32 _pathHash, FILE_NOTIFY_INFORMATION* pNotify) :
		path(_path),
		hash(_pathHash),
		fileName(alloc, pNotify->FileName, reinterpret_cast<WCHAR *>(reinterpret_cast<U8 *>(pNotify->FileName) + pNotify->FileNameLength))
	{
		// We calc hash from: path + "\" + filename.
		// So: "c:\dir\subdir" + "\" + "filename.ext" will have same hash as "c:\dir" + "\" + "subdir\filename.ext"
		hash = JSHash(reinterpret_cast<const U8 *>(Tools::directorySeparator), 1, hash);
		hash = JSHash(pNotify, hash);
	}

	DiskFileName(IMemoryAllocator *alloc, const PathSTR &_path, U32 _pathHash, const WSimpleString::_B &_fileName) :
		path(_path),
		hash(_pathHash),
		fileName(alloc, _fileName)
	{
		hash = JSHash(reinterpret_cast<const U8 *>(Tools::directorySeparator), 1, hash);
		hash = JSHash(_fileName, hash);
	}

	bool operator == (const DiskFileName &b) const
	{
		if (hash == b.hash)
		{
			// almost always it will be true.... but we still have to check
			if (path.size() == b.path.size()) // check when path length i same... simplest case
			{
				return path == b.path && fileName == b.fileName;
			}
			else if ((path.size() + fileName.size()) == (b.path.size() + b.fileName.size())) // total lenght must be same
			{
				// check when path length is different but total leng is still same
				auto aPath = path.begin();
				auto bPath = b.path.begin();
				auto aFN = fileName.begin();
				auto bFN = b.fileName.begin();
				auto aPathSize = path.size();
				auto bPathSize = path.size();
				auto aFNSize = fileName.size();
				auto bFNSize = b.fileName.size();
				I32 dif = static_cast<I32>(aPathSize - bPathSize);

				/* example:
				a : path = c:\AA fileName = BB\stuffs.txt
				b : path = c:\AA\BB fileName = stuffs.txt
				aPathSize = 5
				bPathSize = 8
				dif = -3
				wcsncmp("c:\AA", "c:\AA\BB", 5) = true
				wcsncmp("BB\stuffs.txt"+3, "stuffs.txt", 10) == true // wcsncmp("stuffs.txt"+3, "stuffs.txt", 10)
				we must compare: aFN: "BB\" with bPath: "\BB"
				*/
				// looks complicate... but we check if: path + "\\" + fileName == b.path + "\\" + b.fileName
				if (dif < 0)
				{
					dif = -dif; // dif is alway positive
					if (wcsncmp(aPath, bPath, aPathSize) == 0 &&
						wcsncmp(aFN + dif, bFN, bFNSize) == 0 &&
						wcsncmp(aFN, bPath + bPathSize - (dif - 1), dif - 1) == 0 &&
						aFN[dif - 1] == Tools::directorySeparatorChar &&
						bPath[bPathSize - dif] == Tools::directorySeparatorChar)
					{
						return true;
					}
				}
				else
				{
					if (wcsncmp(aPath, bPath, bPathSize) == 0 &&
						wcsncmp(aFN, bFN + dif, aFNSize) == 0 &&
						wcsncmp(aPath + aPathSize - (dif - 1), bFN, dif - 1) == 0 &&
						bFN[dif - 1] == Tools::directorySeparatorChar &&
						aPath[aPathSize - dif] == Tools::directorySeparatorChar)
					{
						return true;
					}

				}
			}
		}

		return false;
	}

	PathSTR path;
	U32 hash;
	WSimpleString fileName;

	typedef const DiskFileName key_t;
	inline U32 getHash() const { return hash; }
	inline key_t * getKey() const { return this; }
};


/// <summary>
/// Disk file change event.
/// Note 1: First arg of constructor is always allocator. It is used to create strings with desired memory allocator.
/// Note 2: It is "plain" struct. Destructor will delete all strings too. They have pointer to memory allocator.
/// Note 3: This class will be used with queue and hashtable. Both may not call destructor. 
///         All is stored in Blocks, so whole memory block may be "destroyed" at once.
/// </summary>
struct MonitoredDirEvent : DiskFileName
{
	// They are derived from DiskFileName
	//	PathSTR path;
	//	U32 pathHash;
	//	WSimpleString fileName;
	U32 action;
	WSimpleString fileNameRenameTo;

	MonitoredDirEvent(IMemoryAllocator *alloc, MonitoredDirEvent &src) :
		DiskFileName(alloc, src),
		action(src.action),
		fileNameRenameTo(alloc, src.fileNameRenameTo)
	{}

	template<typename T>
	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, U32 _pathHash, FILE_NOTIFY_INFORMATION* pNotify, T &_fileNameRenameOld) :
		DiskFileName(alloc, _path, _pathHash, _fileNameRenameOld),
		action(_action),
		fileNameRenameTo(alloc, pNotify->FileName, reinterpret_cast<WCHAR *>(reinterpret_cast<U8 *>(pNotify->FileName) + pNotify->FileNameLength)),
		{}

	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, U32 _pathHash, FILE_NOTIFY_INFORMATION* pNotify) :
		DiskFileName(alloc, _path, _pathHash, pNotify),
		action(_action),
		fileNameRenameTo()
	{}

	template<typename T>
	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, U32 _pathHash, T &_fileName) :
		DiskFileName(alloc, _path, _pathHash, _fileName),
		action(_action),
		fileNameRenameTo()
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
			*Max = MemmoryAllocatorsPrivate::DebugStats.MaxAllocatedMemory;

		if (Current)
			*Current = MemmoryAllocatorsPrivate::DebugStats.CurrentAllocatedMemory;

		if (Counter)
			*Counter = MemmoryAllocatorsPrivate::DebugStats.TotalAllocateCommands;
	}

	BAMS_EXPORT bool GetMemoryBlocks(void **current, size_t *size, size_t *counter, void **data)
	{
		return MemmoryAllocatorsPrivate::DebugStats.list(current, size, counter, data);
	}

	// ========================================================================

	void TestRingBuffer()
	{
		PathSTR path = L"c:\\cos\\cs";
		U32 pathHash = hash<PathSTR>()(path);		
		WSTR fn = L"nazwaplik.txt";
		WSTR fn2 = L"nazwaplik2.txt";
		WSTR fn3 = L"nazwaplik3.txt";
		WSTR fn4 = L"nazwaplik4.txt";
		WSTR fn5 = L"nazwaplik4.txt";
		WSTR fn6 = L"nazwaplik4.txt";

		queue<MonitoredDirEvent> events(8192);
		hashtable<MonitoredDirEvent> ht;

		// fill events and ht
		MonitoredDirEvent *ev;
		ev = events.push_back(100, path, pathHash, fn);
		ht.append(ev);
		ev = events.push_back(101, path, pathHash, fn2);
		ht.append(ev);
		ev = events.push_back(102, path, pathHash, fn3);
		ht.append(ev);
		ev = events.push_back(200, path, pathHash, fn4);
		ht.append(ev);
		ev = events.push_back(201, path, pathHash, fn5);
		ht.append(ev);
		ev = events.push_back(202, path, pathHash, fn6);
		ht.append(ev);


		// remove all MonitoredDirEvent from events list
		while (auto ev = events.pop_front())
		{
			TRACE("ev: " << ev->action << "\n");
			ht.remove(ev);
			events.release(ev); // release will delete MonitoredDirEvent from memory.
		}


//		hashtable<const MonitoredDirEvent*, const MonitoredDirEvent *>ht2;

/**
		hashtable<PathSTR, WSimpleString> ht;
		ht.insert(L"hi", L"tada");
		ht.insert(L"ha", L"tada 2");
		ht.insert(L"hi ha ho", L"tada 3");
		ht.insert(L"hey ho", L"tada 4");
		
		auto ha = ht.find(L"ha");
		auto hmm = ht.find(L"hmm");
		*/
		Allocators::Blocks _alloc(MemoryAllocatorStatic<>::GetMemoryAllocator(), 8192);

		//		Allocators::RingBuffer _alloc(MemoryAllocatorStatic<>::GetMemoryAllocator(), 8192);

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



		list<MonitoredDirEvent> lt;
		lt.push_back(200, path, pathHash, fn);
		for (auto it = lt.begin(); it != lt.end();) // li.erase(it) will "move" it to next object
		{
			TRACE("ev: " << it->action << "\n");
			it = lt.erase(it);
		}

	}

	BAMS_EXPORT void DoTests()
	{
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
NAMESPACE_BAMS_END