#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

/**
 * Usefull links:
 * https://developersarea.wordpress.com/2014/09/26/win32-file-watcher-api-to-monitor-directory-changes/
 */
NAMESPACE_CORE_BEGIN


// ============================================================================ ResourceBase ===

void ResourceBase::Init(CWSTR path)
{
	_resourceData = nullptr;
	_resourceSize = 0;
	_isLoaded = false;
	_refCounter = 0;

	WSTR normalizedPath = path;
	Tools::NormalizePath(normalizedPath);

	Tools::CreateUUID(UID);
	Path = normalizedPath;
	CWSTR pPathEnd = Path.end();// Path._buf + Path._used;
	CWSTR pPathBegin = Path.begin();// Path._buf;
	CWSTR pEnd = pPathEnd;
	CWSTR pBegin = pPathBegin;
	while (pPathEnd > pPathBegin)
	{
		--pPathEnd;
		if (*pPathEnd == '.')
		{
			pEnd = pPathEnd;
			break;
		}
	}

	while (pPathEnd > pPathBegin)
	{
		--pPathEnd;
		if (*pPathEnd == Tools::directorySeparatorChar)
		{
			pBegin = pPathEnd + 1;
			break;
		}
	}
	Name = STR(pBegin, pEnd);
}

// ============================================================================ ResourceTypeListEntry ===

ResourceFactoryChain * ResourceFactoryChain::First = nullptr;

// ============================================================================ ResourceManager::InternalData ===


struct ResourceManager::InternalData : public MemoryAllocatorGlobal<>
{
	basic_array<ResourceBase *, InternalData> _resources;
	std::thread *_worker;
	bool _killWorkerFlag;

	struct MonitoredDirDesc
	{
		HANDLE hDir;
		PathSTR path;
		U32 type;
		MonitoredDirDesc(HANDLE _hDir, PathSTR &&_path, U32 _type) : hDir(_hDir), path(_path), type(_type) {}
	};

	object_array<MonitoredDirDesc> _monitoredDirs;

	InternalData() : _worker(nullptr), _killWorkerFlag(false) {};
	virtual ~InternalData()
	{
		if (_worker)
		{
			_killWorkerFlag = true;
			_worker->join();
			make_delete<std::thread>(_worker);
		}

		for (auto *pRes : _resources)
		{
			make_delete<ResourceBase>(pRes);
		}
		_resources.clear();
	}

	void AddResource(ResourceBase *res)
	{
		res->ResourceLoad(nullptr, 0);
		_resources.push_back(res);
	}

	void AddResource(CWSTR path)
	{
		auto res = make_new<ResourceBase>();
		res->Init(path);
		_resources.push_back(res);
	}

	void AddDirToMonitor(CWSTR path, U32 type)
	{
		PathSTR sPath(path);
		AddDirToMonitor(std::move(sPath), type);
	}

	void AddDirToMonitor(PathSTR &&sPath, U32 type)
	{
		for (auto &md : _monitoredDirs)
		{
			if (md.path == sPath)
			{
				md.type |= type;
				return; 
			}
		}

		HANDLE hDir = 0;
		_monitoredDirs.push_back( MonitoredDirDesc(hDir, std::move(sPath), type) );
	}

	
	void Load(ResourceBase *res)
	{
		SIZE_T size = 0;
		BYTE *data = nullptr;
		data = Tools::LoadFile(&size, res->Path, res->GetMemoryAllocator());
		res->ResourceLoad(data, size);
	}	

	//Method to start watching a directory. Call it on a separate thread so it wont block the main thread.  
	void WatchDirectory(LPCWSTR path)
	{
		char buf[2048];
		DWORD nRet;
		BOOL result = TRUE;
		char filename[MAX_PATH];
		HANDLE hDir;
		hDir = CreateFile(path, GENERIC_READ | FILE_LIST_DIRECTORY,
			FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
			NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
			NULL);

		if (hDir == INVALID_HANDLE_VALUE)
		{
			return; //cannot open folder
		}

//		lstrcpy(DirInfo[0].lpszDirName, path);
		OVERLAPPED PollingOverlap;

		FILE_NOTIFY_INFORMATION* pNotify;
		int offset;
		PollingOverlap.OffsetHigh = 0;
		PollingOverlap.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
		while (result)
		{
			result = ReadDirectoryChangesW(
				hDir,// handle to the directory to be watched
				&buf,// pointer to the buffer to receive the read results
				sizeof(buf),// length of lpBuffer
				TRUE,// flag for monitoring directory or directory tree
				FILE_NOTIFY_CHANGE_FILE_NAME |
				FILE_NOTIFY_CHANGE_DIR_NAME |
				FILE_NOTIFY_CHANGE_SIZE,
				//FILE_NOTIFY_CHANGE_LAST_WRITE |
				//FILE_NOTIFY_CHANGE_LAST_ACCESS |
				//FILE_NOTIFY_CHANGE_CREATION,
				&nRet,// number of bytes returned
				&PollingOverlap,// pointer to structure needed for overlapped I/O
				NULL);

			WaitForSingleObject(PollingOverlap.hEvent, INFINITE);
			offset = 0;
			int rename = 0;
			do
			{
				pNotify = (FILE_NOTIFY_INFORMATION*)((char*)buf + offset);
				strcpy_s(filename, "");
				int filenamelen = WideCharToMultiByte(CP_ACP, 0, pNotify->FileName, pNotify->FileNameLength / 2, filename, sizeof(filename), NULL, NULL);
				filename[pNotify->FileNameLength / 2] = 0;
				switch (pNotify->Action)
				{
				case FILE_ACTION_ADDED:
					printf("\nThe file is added to the directory: [%s] \n", filename);
					break;
				case FILE_ACTION_REMOVED:
					printf("\nThe file is removed from the directory: [%s] \n", filename);
					break;
				case FILE_ACTION_MODIFIED:
					printf("\nThe file is modified. This can be a change in the time stamp or attributes: [%s]\n", filename);
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
					printf("\nThe file was renamed and this is the old name: [%s]\n", filename);
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					printf("\nThe file was renamed and this is the new name: [%s]\n", filename);
					break;
				default:
					printf("\nDefault error.\n");
					break;
				}

				offset += pNotify->NextEntryOffset;

			} while (pNotify->NextEntryOffset); //(offset != 0);
		}

		CloseHandle(hDir);

	}

	static void Worker(InternalData *rm)
	{
		while (!rm->_killWorkerFlag)
		{
			std::this_thread::sleep_for(std::chrono::milliseconds(100));

			// do stuff in worker thread


		}
	}

	void StartWorker()
	{
		if (!_worker)
		{
			_worker = make_new<std::thread>(Worker, this);
		}
	}
};

// ============================================================================ ResourceManager ===

ResourceManager::ResourceManager()
{
	_data = make_new<InternalData>();
}

ResourceManager::~ResourceManager()
{
	make_delete<InternalData>(_data);
}

ResourceManager * ResourceManager::Create()
{
	return make_new<ResourceManager>();
}

void ResourceManager::Destroy(ResourceManager *rm)
{
	make_delete<ResourceManager>(rm);
}

void ResourceManager::Filter(ResourceBase ** resList, U32 * resCount, CSTR & _pattern)
{
	STR pattern(_pattern);
	I32 counter = 0;
	I32 max = (resCount && resList) ? *resCount : 0;

	for (auto &res : _data->_resources)
	{
		if (res->Name.wildcard(pattern))
		{
			if (counter < max)
				resList[counter] = res;
			++counter;
		}
	}

	if (resCount)
		*resCount = counter;
}

ResourceBase * ResourceManager::Get(const STR & resName, U32 typeId)
{
	for (auto &res : _data->_resources)
	{
		if (res->Name == resName && (res->Type == typeId || typeId == ResourceBase::UNKNOWN || res->Type == ResourceBase::UNKNOWN))
		{
			if (res->Type == ResourceBase::UNKNOWN && typeId != ResourceBase::UNKNOWN)
			{
				ResourceFactoryChain *f = ResourceFactoryChain::First;
				while (f)
				{
					if (f->TypeId == typeId)
					{
						res->_resourceImplementation = f->Create();
						break;
					}
				}
			}

			return res;
		}
	}

	// no resource... create one
	auto res = make_new<ResourceBase>();
	res->Name = resName;
	res->Type = typeId;
	_data->AddResource(res);

	return res;
}

ResourceBase * ResourceManager::Get(const UUID & resUID)
{
	for (auto &res : _data->_resources)
	{
		if (res->UID == resUID)
			return res;
	}

	// no resource... create one
	auto res = make_new<ResourceBase>();
	_data->AddResource(res);

	return res;
}


void ResourceManager::Add(CWSTR path)
{
	_data->AddResource(path);
}

void ResourceManager::LoadSync()
{
	for (auto &res : _data->_resources)
	{
		if (!res->isLoaded())
		{
			_data->Load(res);
			if (res->isLoaded())
			{
				res->Update();
			}
		}
	}
}

void ResourceManager::LoadAsync()
{
}


// ============================================================================ ResourceManagerModule ===

ResourceManager *globalResourceManager = nullptr;
void ResourceManagerModule::Update(float dt)
{

}

void ResourceManagerModule::Initialize()
{
	globalResourceManager = ResourceManager::Create();
}

void ResourceManagerModule::Finalize()
{
	ResourceManager::Destroy(globalResourceManager);
	globalResourceManager = nullptr;
}

void ResourceManagerModule::SendMessage(Message *msg)
{
	switch (msg->id)
	{
	case RESOURCEMANAGER_ADD_FILE:
		break;
	}
}

ResourceManagerModule::~ResourceManagerModule()
{
	if (globalResourceManager)
	{
		TRACE("Critical Error: ResourceManagerModule destroyed before Finalize was called.");
	}
}


NAMESPACE_CORE_END