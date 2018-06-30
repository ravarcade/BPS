#include "stdafx.h"

NAMESPACE_CORE_BEGIN


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

	template<typename T>
	DiskFileName(IMemoryAllocator *alloc, const PathSTR &_path, U32 _pathHash, T &_fileName) :
		path(_path),
		hash(_pathHash),
		fileName(alloc, _fileName)
	{
		hash = JSHash(reinterpret_cast<const U8 *>(Tools::directorySeparator), 1, hash);
		hash = JSHash(_fileName, hash);
	}

	bool operator == (const DiskFileName &b)
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
		DiskFileName(alloc,_path, _pathHash, _fileNameRenameOld),
		action(_action),
		fileNameRenameTo(alloc, pNotify->FileName, reinterpret_cast<WCHAR *>(reinterpret_cast<U8 *>(pNotify->FileName) + pNotify->FileNameLength))
		{}

	MonitoredDirEvent(IMemoryAllocator *alloc, U32 _action, const PathSTR &_path, U32 _pathHash, FILE_NOTIFY_INFORMATION* pNotify) :
		DiskFileName(alloc, _path, _pathHash, pNotify),
		action(_action),
		fileNameRenameTo()
	{}

};


// ============================================================================ MonitoredDir ===

struct MonitoredDir
{
	typedef queue<MonitoredDirEvent> EventQueue;

	PathSTR path;
	U32 pathHash;
	PathSTR renameOldTmp;
	U32 type;
	HANDLE hDir;
	OVERLAPPED pollingOverlap;
	char buffer[4096];
	DWORD bufferSize;
	EventQueue *events;
	
	MonitoredDir(PathSTR &&_path, U32 _type, EventQueue *_eventsFromOs) :
		hDir(0), 
		path(_path), 
		pathHash(hash<PathSTR>()(path)),
		type(_type), 
		events(_eventsFromOs), 
		bufferSize(0) 
	{ 
		memset(&pollingOverlap, 0, sizeof(pollingOverlap)); 
	}

	static void CALLBACK OnStartMonitoredDir(__in  ULONG_PTR arg)
	{
		MonitoredDir *_self = reinterpret_cast<MonitoredDir *>(arg);
	}

	void Start()
	{
		if (hDir)
			return;

		// open dir for monitoring
		hDir = CreateFile(path.c_str(),
			GENERIC_READ | FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL,
			OPEN_EXISTING,
			FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);


		if (hDir != INVALID_HANDLE_VALUE)
			BeginRead();
	}

	void Stop()
	{
		if (!hDir && hDir != INVALID_HANDLE_VALUE)
		{
			CancelIo(hDir);
			CloseHandle(hDir);
		}
		hDir = 0;
	}

	void BeginRead()
	{
		// begin read
		memset(&pollingOverlap, 0, sizeof(pollingOverlap));
		bufferSize = 0;
		pollingOverlap.hEvent = this;

		BOOL success = ::ReadDirectoryChangesW(
			hDir,
			&buffer,
			sizeof(buffer),
			TRUE, // monitor children?
			FILE_NOTIFY_CHANGE_LAST_WRITE
			| FILE_NOTIFY_CHANGE_CREATION
			| FILE_NOTIFY_CHANGE_FILE_NAME
			| FILE_NOTIFY_CHANGE_SIZE
			| FILE_NOTIFY_CHANGE_DIR_NAME,
			&bufferSize,
			&pollingOverlap,
			&NotificationCompletion);

	}

	static void CALLBACK NotificationCompletion(
		DWORD dwErrorCode,									// completion code
		DWORD dwNumberOfBytesTransfered,					// number of bytes transferred
		LPOVERLAPPED lpOverlapped)							// I/O information buffer
	{
		MonitoredDir* md = (MonitoredDir*)lpOverlapped->hEvent;

		if (dwErrorCode == ERROR_OPERATION_ABORTED)
		{
			TRACE("NotificationCompletion: dwErrorCode == ERROR_OPERATION_ABORTED ... EXIT?");
			return;
		}

		if (!dwNumberOfBytesTransfered) // !? what? dwNumberOfBytesTransfered must be > (3x sizeof(DWORD) + 1x sizeof(WCHAR) and we test if it is ZERO?
		{
			TRACE("NotificationCompletion: dwNumberOfBytesTransfered = 0 ... EXIT?");
			md->BeginRead();
			return;
		}

		// Can't use sizeof(FILE_NOTIFY_INFORMATION) because
		// the structure is padded to 16 bytes.
		_ASSERTE(dwNumberOfBytesTransfered >= offsetof(FILE_NOTIFY_INFORMATION, FileName) + sizeof(WCHAR));


		// ... process events ...
		FILE_NOTIFY_INFORMATION* pNotify;
		int offset = 0;
		int rename = 0;
		do
		{
			pNotify = (FILE_NOTIFY_INFORMATION*)((char*)md->buffer + offset);
			
			switch (pNotify->Action)
			{
			case FILE_ACTION_ADDED:
				md->events->push_back(pNotify->Action, md->path, md->pathHash, pNotify);
				break;
			case FILE_ACTION_REMOVED:
				md->events->push_back(pNotify->Action, md->path, md->pathHash, pNotify);
				break;
			case FILE_ACTION_MODIFIED:
				md->events->push_back(pNotify->Action, md->path, md->pathHash, pNotify);
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				md->renameOldTmp = PathSTR(pNotify->FileName, reinterpret_cast<wchar_t*>((reinterpret_cast<U8*>(pNotify->FileName) + pNotify->FileNameLength)));
				break;
			case FILE_ACTION_RENAMED_NEW_NAME:
				md->events->push_back(pNotify->Action, md->path, md->pathHash, pNotify, md->renameOldTmp.ToBasicString());
				break;
			default:
				TRACE("\nDefault error.\n");
				break;
			}

			offset += pNotify->NextEntryOffset;
		} while (pNotify->NextEntryOffset);

		// begin read again.
		md->BeginRead();
	}
};

// ============================================================================ DirectoryChangeNotifier::InternalData ===

struct DirectoryChangeNotifier::InternalData : public MemoryAllocatorStatic<>
{

private:
	std::thread *_worker;
	bool _killWorkerFlag;
	std::mutex _monitoredDirsAccessMutex;
	basic_array<MonitoredDir *, Allocator>_monitoredDirs;
	MonitoredDir::EventQueue _eventsFromOs;

	struct DelayedEvent : MonitoredDirEvent
	{
		DelayedEvent(IMemoryAllocator *alloc, MonitoredDirEvent &src) :
			MonitoredDirEvent(alloc, src),
			_waitTo(clock::now()),
			_nextOnList(nullptr),
			_prevOnList(nullptr)
		{}

		DelayedEvent(time waitTo, IMemoryAllocator *alloc, MonitoredDirEvent &src) :
			MonitoredDirEvent(alloc, src),
			_waitTo(waitTo),
			_nextOnList(nullptr),
			_prevOnList(nullptr)
		{}

		DelayedEvent(clock::duration delay, IMemoryAllocator *alloc, MonitoredDirEvent &src) :
			MonitoredDirEvent(alloc, src),
			_waitTo(clock::now() + delay),
			_nextOnList(nullptr),
			_prevOnList(nullptr)
		{}

		time _waitTo;
		DelayedEvent * _nextOnList;
		DelayedEvent * _prevOnList;
	};

	typedef list<DelayedEvent> ListOfDelayedEvents;
	typedef hashtable<DiskFileName *, DelayedEvent*> UniqEvenTable;
	ListOfDelayedEvents _eventsToProcess;
	UniqEvenTable _uniqEvenTable;



public:
	typedef queue<MonitoredDirEvent> EventQueue;

	InternalData() :
		_worker(nullptr),
		_killWorkerFlag(false),
		_eventsFromOs(8192) // 8kB in event ring buffer
	{}

	virtual ~InternalData()
	{
		StopWorker();

		// delete list of all monitored dirs
		std::lock_guard<std::mutex> lck(_monitoredDirsAccessMutex);
		for (auto md : _monitoredDirs)
			make_delete<MonitoredDir>(md);
	}

	void AddDir(PathSTR &&sPath, U32 type)
	{
		for (auto &md : _monitoredDirs)
		{
			if (md && md->path == sPath)
			{
				md->type |= type;
				return;
			}
		}

		auto newMonitoredDir = make_new<MonitoredDir>(std::move(sPath), type, &_eventsFromOs);
		std::lock_guard<std::mutex> lck(_monitoredDirsAccessMutex);
		_monitoredDirs.push_back(newMonitoredDir);

		newMonitoredDir->Start();
	}

	void RemoveDir(PathSTR &&sPath)
	{
		std::lock_guard<std::mutex> lck(_monitoredDirsAccessMutex);
		for (auto &md : _monitoredDirs)
		{
			if (md && md->path == sPath)
			{
				md->Stop();
				make_delete<MonitoredDir>(md);
				md = nullptr;
				return;
			}
		}
	}

	static void Worker(InternalData *rm)
	{
		// reset statistics
		rm->addcount = 0;
		rm->remcount = 0;
		rm->modcount = 0;
		rm->rencount = 0;
		auto &events = rm->_eventsFromOs;
		while (!rm->_killWorkerFlag)
		{
			SleepEx(1000, TRUE); // we do nothing... everything is done in os
			while (auto ev = events.pop_front())
			{
				rm->ProcessEvent(ev);           // .... process ....
				events.release(ev);				// importan part: here is relesed memory used to store event.
			}

			wprintf(L"Add: [%d], Del: [%d], Mod: [%d], Ren: [%d]\n", rm->addcount, rm->remcount, rm->modcount, rm->rencount);
		}

	}

	int addcount, remcount, modcount, rencount;

	/// <summary>
	/// Processes the event.
	/// </summary>
	/// <param name="ev">The event.</param>
	void ProcessEvent(MonitoredDirEvent *ev)
	{
		clock::duration delays[] = {
			0s,    // 0
			250ms, // FILE_ACTION_ADDED
			0s,    // FILE_ACTION_REMOVED
			250ms, // FILE_ACTION_MODIFIED
			0s,    // FILE_ACTION_RENAMED_OLD_NAME
			0s,    // FILE_ACTION_RENAMED_NEW_NAME
		};

		DiskFileName *key = ev;

//		auto de = _uniqEvenTable.find(key);
//		if (!de) // insert
		{
//			auto etp = _eventsToProcess.push_back(*ev);
//			MonitoredDirEvent::Key key(etp);
//			_uniqEvenTable.insert(key, etp);

		}
	
		switch (ev->action)
		{
		case FILE_ACTION_ADDED:
			wprintf(L"Add: [%s] \n", ev->fileName.c_str());
			++addcount;
			break;
		case FILE_ACTION_REMOVED:
			wprintf(L"Delete: [%s] \n", ev->fileName.c_str());
			++remcount;
			break;
		case FILE_ACTION_MODIFIED:
			wprintf(L"Modify: [%s]\n", ev->fileName.c_str());
			++modcount;
			break;
		case FILE_ACTION_RENAMED_NEW_NAME:
			wprintf(L"Rename: [%s] -> [%s]\n", ev->fileNameRenameTo.c_str(), ev->fileName.c_str());
			++rencount;
			break;
		case FILE_ACTION_RENAMED_OLD_NAME:
		default:
			wprintf(L"!!! Default error.\n");
			break;
		}
	}

	void StartWorker()
	{
		if (!_worker)
			_worker = make_new<std::thread>(Worker, this);

		for (auto &md : _monitoredDirs)
		{
			if (md)
				md->Start();
		}
	}

	void StopWorker()
	{
		if (_worker)
		{
			_killWorkerFlag = true;

			for (auto &md : _monitoredDirs)
			{
				if (md)
					md->Stop();
			}

			_worker->join();
			make_delete<std::thread>(_worker);
			_worker = nullptr;
		}
	}

	void StartMonitoredDir(MonitoredDir *md)
	{
		QueueUserAPC(MonitoredDir::OnStartMonitoredDir, _worker->native_handle(), reinterpret_cast<ULONG_PTR>(md));
	}

};

// ============================================================================ DirectoryChangeNotifier ===


DirectoryChangeNotifier::DirectoryChangeNotifier() { _data = make_new<InternalData>(); }
DirectoryChangeNotifier::~DirectoryChangeNotifier() { make_delete<InternalData>(_data); }
void DirectoryChangeNotifier::AddDir(PathSTR &&sPath, U32 type) { _data->AddDir(std::move(sPath), type); }
void DirectoryChangeNotifier::Start() { _data->StartWorker(); }
void DirectoryChangeNotifier::Stop() { _data->StopWorker(); }

NAMESPACE_CORE_END