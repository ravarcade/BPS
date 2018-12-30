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

	// required to use DiskFileName in hash table and as hash table key
	typedef const DiskFileName key_t;
	inline U32 getHash() const { return hash; }
	inline key_t * getKey() const { return this; }
};


/// <summary>
/// Disk file change event.
/// Note 1: First arg of constructor is always allocator. It is used to create strings with desired memory allocator.
/// Note 2: It is "plain" struct. Destructor will delete all strings too. Strings have pointer to own memory allocator.
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
	bool isStopped;
	
	MonitoredDir(PathSTR &&_path, U32 _type, EventQueue *_eventsFromOs) :
		hDir(0),
		path(_path),
		pathHash(hash<PathSTR>()(path)),
		type(_type),
		events(_eventsFromOs),
		bufferSize(0),
		isStopped(true)
	{ 
		memset(&pollingOverlap, 0, sizeof(pollingOverlap)); 
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
		if (hDir && hDir != INVALID_HANDLE_VALUE)
		{
			CancelIo(hDir);
			int cnt = 100;
			while (!isStopped && cnt > 0)
			{
				SleepEx(5, TRUE);
				--cnt;
			}
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
		isStopped = false;
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
		MonitoredDir* md = lpOverlapped && lpOverlapped->hEvent ? (MonitoredDir*)lpOverlapped->hEvent : nullptr;
		if (dwErrorCode == ERROR_OPERATION_ABORTED && md)
		{
			md->isStopped = true;
			return;
		}
		if (dwErrorCode != ERROR_SUCCESS || lpOverlapped == 0 || lpOverlapped->hEvent == 0)
		{
			TRACE("NotificationCompletion: dwErrorCode = " << dwErrorCode << " ... EXIT?");
			return;
		}

//		MonitoredDir* md = (MonitoredDir*)lpOverlapped->hEvent;

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

struct DirectoryChangeNotifier::InternalData : public Allocators::Ext<>
{

private:
	std::mutex _monitoredDirsAccessMutex;
	basic_array<MonitoredDir *, Allocators::default> _monitoredDirs;
	MonitoredDir::EventQueue _eventsQueue;
	WSTR _fileName;
	WSTR _fileNameRenameTo;

public:
//	typedef queue<MonitoredDirEvent> EventQueue;

	InternalData() :
		_fileName(1000),
		_fileNameRenameTo(224),
		_eventsQueue()
	{}

	virtual ~InternalData()
	{
		StopWorker();

		// delete list of all monitored dirs
		std::lock_guard<std::mutex> lck(_monitoredDirsAccessMutex);
		for (auto md : _monitoredDirs)
			make_delete<MonitoredDir>(md);
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

	// ==================================================================== inherited DirectoryChangeNotifier ===

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

		auto newMonitoredDir = make_new<MonitoredDir>(std::move(sPath), type, &_eventsQueue);
		std::lock_guard<std::mutex> lck(_monitoredDirsAccessMutex);
		_monitoredDirs.push_back(newMonitoredDir);

		newMonitoredDir->Start();
	}

	void StartWorker()
	{
		for (auto &md : _monitoredDirs)
		{
			if (md)
				md->Start();
		}
	}

	void StopWorker()
	{
		for (auto &md : _monitoredDirs)
		{
			if (md)
				md->Stop();
		}
	}

	DiskEvent *GetDiskEvent()
	{
		DiskEvent *ret = nullptr;
		static DiskEvent de;

		if (auto ev = _eventsQueue.pop_front()) // get event from queue
		{
			// .... process ....
			// Copy and combine path and filename to _fileName ... and fileNameRenameTo to _fileNameRenameTo.
			// Both variable will hold data after we destroy orginal event container.
			// Here string will be copied. There is no reason to optimise this.
			_fileName = ev->path + Tools::wDirectorySeparator + ev->fileName;
			_fileNameRenameTo = ev->fileNameRenameTo;

			// fill container "DiskEvent" with data about event
			de.action = ev->action;
			de.fileName = _fileName;
			de.fileNameRenameTo = _fileNameRenameTo;

			ret = &de;
			_eventsQueue.release(ev);				// importan part: here memory used by event is relesed.
		}
		return ret;
	}
};

// ============================================================================ DirectoryChangeNotifier ===


DirectoryChangeNotifier::DirectoryChangeNotifier() { _data = make_new<InternalData>(); }
DirectoryChangeNotifier::~DirectoryChangeNotifier() { make_delete<InternalData>(_data); }
void DirectoryChangeNotifier::AddDir(PathSTR &&sPath, U32 type) { _data->AddDir(std::move(sPath), type); }
void DirectoryChangeNotifier::Start() { _data->StartWorker(); }
void DirectoryChangeNotifier::Stop() { _data->StopWorker(); }
DiskEvent *DirectoryChangeNotifier::GetDiskEvent() { return _data->GetDiskEvent(); }

NAMESPACE_CORE_END