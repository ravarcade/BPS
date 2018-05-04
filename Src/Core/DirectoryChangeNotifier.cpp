#include "stdafx.h"

NAMESPACE_CORE_BEGIN

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

typedef base_queue_threadsafe<MonitoredDirEvent> MonitoredDirsEventQueue;

// ============================================================================ MonitoredDir ===

struct MonitoredDir
{
	PathSTR path;
	PathSTR renameOldTmp;
	U32 type;
	HANDLE hDir;
	OVERLAPPED pollingOverlap;
	char buffer[2048];
	DWORD bufferSize;
	MonitoredDirsEventQueue *events;
	
	MonitoredDir(PathSTR &&_path, U32 _type, MonitoredDirsEventQueue *_events) : hDir(0), path(_path), type(_type), events(_events), bufferSize(0) { memset(&pollingOverlap, 0, sizeof(pollingOverlap)); }

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
				md->events->push_back(pNotify->Action, md->path, pNotify);
				break;
			case FILE_ACTION_REMOVED:
				md->events->push_back(pNotify->Action, md->path, pNotify);
				break;
			case FILE_ACTION_MODIFIED:
				md->events->push_back(pNotify->Action, md->path, pNotify);
				break;
			case FILE_ACTION_RENAMED_OLD_NAME:
				md->renameOldTmp = PathSTR(pNotify->FileName, reinterpret_cast<wchar_t*>((reinterpret_cast<U8*>(pNotify->FileName) + pNotify->FileNameLength)));
				break;
			case FILE_ACTION_RENAMED_NEW_NAME:
				md->events->push_back(pNotify->Action, md->path, pNotify, md->renameOldTmp.ToBasicString());
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

struct DirectoryChangeNotifier::InternalData : public MemoryAllocatorGlobal<>
{

private:
	std::thread *_worker;
	bool _killWorkerFlag;
	std::mutex _monitoredDirsAccessMutex;
	basic_array<MonitoredDir *, Allocator>_monitoredDirs;
	MonitoredDirsEventQueue _events;



public:
	typedef base_ringbuffer<MonitoredDirEvent> EventQueue;

	InternalData() :
		_worker(nullptr),
		_killWorkerFlag(false),
		_events(8192) // 8kB in event ring buffer
	{
	}

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

		auto newMonitoredDir = make_new<MonitoredDir>(std::move(sPath), type, &_events);
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
		rm->addcount = 0;
		rm->remcount = 0;
		rm->modcount = 0;
		rm->rencount = 0;
		while (!rm->_killWorkerFlag)
		{
			SleepEx(1000, TRUE); // we do nothing... everything is done in os
			while (!rm->_events.empty())
			{
				auto ev = rm->_events.lock_and_front();
				rm->ProcessEvent(ev);
				rm->_events.pop_and_unlock();
			}
			wprintf(L"Add: [%d], Del: [%d], Mod: [%d], Ren: [%d]\n", rm->addcount, rm->remcount, rm->modcount, rm->rencount);
		}

	}

	int addcount, remcount, modcount, rencount;

	void ProcessEvent(MonitoredDirEvent *ev)
	{
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
			wprintf(L"Rename: [%s] -> [%s]\n", ev->fileNameRenameOld.c_str(), ev->fileName.c_str());
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