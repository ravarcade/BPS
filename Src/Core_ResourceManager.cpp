#include "stdafx.h"

/*
 ResourceBase:
 1. Type = UNKNOWN               : Added to resource manager but not recognized yet.
 2. Type = RawData::GetTypeId()  : All resources not matching any resonable resource type.
 3. Type = RealResourceType::GetTypeId()

 ResourceBase states:
 [1] UNKNOW - Starting resource type and state. Go to [2]. Need to call ResourceManager::FindType or use RecourceManager::Get<Type>
	   _isLoaded = false
	   ** Actions: 
		 - Update - nothing, 
		 - Delete - remove from resources, 

 [2] Recognized but not used and not loaded 
	   _isLoaded = false
	   _refCounter == 0
	   type != UNKNOWN
	   ** Actions:
		 - Update - nothing,
		 - Delete - remove from resources,

 [3] Recognized, not loaded, used. Go to [4]. ResourceManager::Load(res), res->Update
	   _isLoaded = false
	   _refCounter > 0
	   type != UNKNOWN
	   ** Actions:
	   - Update - wait with load,
	   - Delete - do not load, mark deleted (use generic resource), do not remove from resources

 [4] Recognized, loaded, used.
	   _isLoaded = true
	   _refCounter > 0 
	   type != UNKNOW
	   - Update - wait with load, use generic resource,
	   - Delete - 

 [5] Modified, loaded, used

 */

/*
 TODO:
 - odczyt asynchroniczny zasobów u¿ytych (_refCounter > 0)
 - modifikowanie u¿ytego zasobu (_refCounter > 0), po usuniêciu pliku tak by je zast¹piæ zasobem "generic" dla danego typu
 - modifikowanie u¿ytego zasobu (_refCounter > 0), reload zmodifikowanego zasobu
 - zwalnianie zasobu (_refCounter = 0)
 - usuwanie zasobu nieu¿ywanego(_refCounter = 0), czyli zwalniamy go z pamiêci ale nie kasujemy z listy zasobów.
 - modifikowanie zasobu nieu¿ywanego 
*/

/**
 * Usefull links:
 * https://developersarea.wordpress.com/2014/09/26/win32-file-watcher-api-to-monitor-directory-changes/
 */
NAMESPACE_CORE_BEGIN

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;
const wchar_t *MANIFESTFILENAME = L"resource_manifest.xml";

// ============================================================================ ResourceBase ===
const time::duration ResourceBase::defaultDelay = 500ms;

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
	CWSTR pPathEnd = Path.end();
	CWSTR pPathBegin = Path.begin();
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
		if (*pPathEnd == Tools::directorySeparatorChar)
		{
			++pPathEnd;
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

// ============================================================================ ResourceFactoryChain ===

ResourceFactoryChain * ResourceFactoryChain::First = nullptr;

// ============================================================================ ResourceManager::InternalData ===


struct ResourceManager::InternalData : public MemoryAllocatorStatic<>
{
	basic_array<ResourceBase *, Allocator> _resources;
	DirectoryChangeNotifier _monitoredDirs;
	std::thread *_worker;
	bool _killWorkerFlag;
	std::mutex _fileLoadingMutex;
	WSTR _rootDir;
	WSTR _manifestFileName;

	InternalData() :
		_worker(nullptr),
		_killWorkerFlag(false)
	{};

	virtual ~InternalData()
	{
		StopMonitoring();
		SaveManifest();
		for (auto *pRes : _resources)
		{
			make_delete<ResourceBase>(pRes);
		}
		_resources.clear();
	}
	
	ResourceBase *AddResource(ResourceBase *res)
	{
		res->ResourceLoad(nullptr, 0);
		_resources.push_back(res);
		return res;
	}

	bool IsUniqFile(CWSTR path)
	{
		return FilterByFilename(path) == nullptr;
	}

	ResourceBase *AddResource(CWSTR path)
	{
		if (!IsUniqFile(path))
			return nullptr;

		auto res = make_new<ResourceBase>();
		res->Init(path);
		_resources.push_back(res);
		return res;
	}

	ResourceBase *FilterByFilename(WSTR &filename)
	{
		for (auto &res : _resources)
		{
			if (res->Path == filename)
			{
				return res;
			}
		}
		return nullptr;
	}


	ResourceBase *FilterByFilename(CWSTR filename)
	{
		WSTR fn(filename);
		return FilterByFilename(fn);
	}

	bool FindResourceByFilename(ResourceBase **&out, const WSTR &filename)
	{
		auto f = out ? ++out : _resources.begin();
		auto l = _resources.end();

		while (f != l)
		{
			if ((*f)->Path.startWith(filename) && ((*f)->Path.size() == filename.size() || (*f)->Path[filename.size()] == Tools::directorySeparatorChar))
			{
				out = f;
				return true;
			}
			++f;
		}

		return false;
	}


	// check all resources if they are modified or removed
	void CheckForUpdates()
	{
		auto waitTime = clock::now() + ResourceBase::defaultDelay;
		for (auto res : _resources)
		{
			SIZE_T resSize = 0;
			time_t resTime = 0;
			if (!Tools::InfoFile(&resSize, &resTime, res->Path))
			{
				// removed!? Do second check
				if (!Tools::Exist(res->Path))
					res->_isDeleted = true;
					
			}
			else
			{
				if (res->_fileTimeStamp != resTime ||
					res->_resourceSize != resSize)
				{
					// modified
					res->_waitWithUpdate = waitTime;
					res->_isModified = true;
				}
			}
		}
	}

	static void SearchForFilesCallback(WSTR &filename, U64 fileSize, time_t timestamp, void *ctrl)
	{
		auto rm = reinterpret_cast<ResourceManager::InternalData *>(ctrl);
		if (!(filename == rm->_manifestFileName))
			rm->AddResource(filename.c_str());

	}

	void ScanRootdir()
	{
		Tools::SearchForFiles(_rootDir, &SearchForFilesCallback, this);
	}

	void UpdateResource(const WSTR &filename)
	{
		auto waitTime = clock::now() + ResourceBase::defaultDelay;
		for (ResourceBase **res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!(*res)->_isDeleted)
			{
				(*res)->_waitWithUpdate = waitTime;
				(*res)->_isModified = true;
			}
		}
	}

	void RemoveResource(const WSTR &filename)
	{
		for (ResourceBase **res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!(*res)->_isDeleted)
			{
				(*res)->_isDeleted = true;
			}
		}
	}

	void RenameResource(const WSTR &filename, const WSTR newfilename)
	{
		I32 fnlen = static_cast<I32>(filename.size());
		WSTR nfn = newfilename;
		for (int i = fnlen - 1; i >= 0; --i)
		{
			if (filename[i] == Tools::directorySeparatorChar)
			{
				nfn = filename.substr(0, i + 1);
				nfn += newfilename;
				break;
			}
		}
		if (_rootDir.startWith(filename) && (_rootDir.size() == fnlen || _rootDir[fnlen] == Tools::directorySeparatorChar))
		{
			_rootDir = nfn + _rootDir.substr(fnlen);
		}
		for (ResourceBase **res = nullptr; FindResourceByFilename(res, filename); )
		{
			(*res)->Path = nfn + (*res)->Path.substr(fnlen);
		}
	}

	void AddDirToMonitor(CWSTR path, U32 type)
	{
		PathSTR sPath(path);
		AddDirToMonitor(std::move(sPath), type);
	}

	void AddDirToMonitor(PathSTR &&sPath, U32 type) 
	{ 
		_monitoredDirs.AddDir(std::move(sPath), type); 
	}

	void RootDir(CWSTR path)
	{
		AddDirToMonitor(path, 0);
		_rootDir = path;
		Tools::NormalizePath(_rootDir);
		LoadManifest();
		ScanRootdir();
	}
	
	void CreateResourceImplementation(ResourceBase *res)
	{
		// we must have resource implemented
		if (res->_resourceImplementation == nullptr)
		{
			// we must have res type set
			if (res->Type == ResourceBase::UNKNOWN)
				res->Type = RawData::GetTypeId();

			// we need to find correct "create" function ...
			for (auto f = ResourceFactoryChain::First; f; f = f->Next)
			{
				if (f->TypeId == res->Type)
				{
					// ... an create it
					res->_resourceImplementation = f->Create();
					break;
				}
			}
		}
	}

	void SetResourceType(ResourceBase *res, U32 resTypeId)
	{
		if (res->Type != ResourceBase::UNKNOWN)
		{
			// delete old resource
			res->_resourceImplementation->GetFactory()->Destroy(res->_resourceImplementation);
		}

		res->Type = resTypeId;
		if (resTypeId != ResourceBase::UNKNOWN)
		{
			// we need to find correct "create" function ...
			for (auto f = ResourceFactoryChain::First; f; f = f->Next)
			{
				if (f->TypeId == resTypeId)
				{
					// ... an create it
					res->_resourceImplementation = f->Create();
					break;
				}
			}
		}
	}

	void Load(ResourceBase *res)
	{
		if (res->_isDeleted || (res->_isModified && res->_waitWithUpdate > clock::now()))
			return;

		SIZE_T size = 0;
		BYTE *data = nullptr;

		if (res->Type == ResourceBase::UNKNOWN) 
			res->_resourceImplementation->Release(res);

		CreateResourceImplementation(res);
	
		data = Tools::LoadFile(&size, &res->_fileTimeStamp, res->Path, res->GetMemoryAllocator());
		

		// mark resource as deleted if file not exist and can't be loaded
		if (data == nullptr && size == 0)
			res->_isDeleted = !Tools::Exist(res->Path);

		if (!res->_isDeleted)
		{
			res->ResourceLoad(data, size);
			res->_isModified = false;
		}
	}	

	WSTR fileName, fileNameRenameTo;
	int addcount, remcount, modcount, rencount;
	static void Worker(InternalData *rm)
	{
		// reset statistics
		rm->addcount = 0;
		rm->remcount = 0;
		rm->modcount = 0;
		rm->rencount = 0;

		while (!rm->_killWorkerFlag)
		{
			// this will wait 1 second for new events to process
			SleepEx(1000, TRUE); // we do nothing... everything is done in os
			while (auto ev = rm->_monitoredDirs.GetDiskEvent()) // we read all events
			{
				auto &fn = rm->fileName;
				auto &frn = rm->fileNameRenameTo;
				fn = ev->fileName;
				frn = ev->fileNameRenameTo;

				switch (ev->action)
				{
				case FILE_ACTION_ADDED:
					if (!Tools::IsDirectory(rm->fileName))
					{
						if (rm->AddResource(rm->fileName.c_str()))
						{
							wprintf(L"Add: [%s] \n", fn.c_str());
							++rm->addcount;
						}
					}
					break;
				case FILE_ACTION_REMOVED:
					rm->RemoveResource(rm->fileName.c_str());
					wprintf(L"Delete: [%s] \n", fn.c_str());
					++rm->remcount;
					break;
				case FILE_ACTION_MODIFIED:
					if (!Tools::IsDirectory(rm->fileName))
					{
						rm->UpdateResource(fn);
						wprintf(L"Modify: [%s]\n", fn.c_str());
						++rm->modcount;
					}
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					rm->RenameResource(fn, frn);
					wprintf(L"Rename: [%s] -> [%s]\n", fn.c_str(), frn.c_str());
					++rm->rencount;
					break;
				case FILE_ACTION_RENAMED_OLD_NAME:
				default:
					wprintf(L"!!! Default error.\n");
					break;
				}
			}
			rm->ProcessResources();
			wprintf(L"Still alive... Add: [%d], Del: [%d], Mod: [%d], Ren: [%d]\n", rm->addcount, rm->remcount, rm->modcount, rm->rencount);
		}
	}


	bool ProcessResources()
	{
		bool allLoaded = true;
		std::lock_guard<std::mutex> lck(_fileLoadingMutex);
		auto now = clock::now();
		int cnt = 0;
		for (auto &res : _resources)
		{
			++cnt;
			if (res->_isLoaded && (res->_isDeleted || (res->_isModified && res->_waitWithUpdate > now)))
			{
				res->GetImplementation()->Release(res);
			}

			if (!res->_isLoaded && res->_refCounter && !res->_isDeleted && (!res->_isModified || res->_waitWithUpdate < now))
			{
				wprintf(L"Loading: %s\n", res->Path.c_str());
				Load(res);
				if (res->_isLoaded)
				{
					res->Update();
				}
			}
			if (res->_refCounter && !res->_isLoaded)
			{
				allLoaded = false;
			}
		}
		return allLoaded;
	}

	void LoadEverything()
	{
		bool allLoaded = ProcessResources();
		while (!_killWorkerFlag && !allLoaded)
		{
			SleepEx(100, TRUE);
			allLoaded = ProcessResources();
		}
	}

	void StartMonitoring()
	{
		_monitoredDirs.Start();

		if (!_worker)
			_worker = make_new<std::thread>(Worker, this);
	}

	void StopMonitoring()
	{
		if (_worker)
		{
			_killWorkerFlag = true;
			_worker->join();
			make_delete<std::thread>(_worker);
			_worker = nullptr;
		}

		_monitoredDirs.Stop();
	}

	void LoadManifest()
	{
		WSTR rpath = _rootDir + Tools::wDirectorySeparator;
		_manifestFileName = rpath + MANIFESTFILENAME;
		SIZE_T manifestSize = 0;
		WSTR cvt;
		STR fn;


		char *manifest = reinterpret_cast<char *>(Tools::LoadFile(&manifestSize, nullptr, _manifestFileName, GetMemoryAllocator()));
		if (manifest)
		{
			XMLDocument xml;
			xml.Parse(manifest, manifestSize);

			auto root = xml.FirstChildElement("ResourcesManifest");
			auto qty = root->IntAttribute("qty");
			for (auto r = root->FirstChildElement("Resource"); r; r = r->NextSiblingElement())
			{
				fn = r->GetText();
				cvt.UTF8(fn);
				WSTR wfn = (cvt.size() > 2 && cvt[1] == ':' && cvt[2] == Tools::directorySeparatorChar) ?  cvt : rpath + cvt;

				auto res = make_new<ResourceBase>();
				res->Path = wfn;
				res->Name = r->Attribute("Name");
				SetResourceType(res, r->UnsignedAttribute("Type", ResourceBase::UNKNOWN));
				const char *uid = r->Attribute("UID");
				Tools::String2UUID(res->UID, uid);
				res->_fileTimeStamp = r->Int64Attribute("Update", 0);
				res->_resourceSize = r->Int64Attribute("Size", 0);
				_resources.push_back(res);
			}
		}
		if (manifest)
			GetMemoryAllocator()->deallocate(manifest);
	}

	void SaveManifest()
	{
		XMLDocument out;
		auto *rm = out.NewElement("ResourcesManifest");
		rm->SetAttribute("qty", (int)_resources.size());
		STR cvt;
		char uidbuf[40];
		WSTR rpath = _rootDir + Tools::wDirectorySeparator;
		U32 rplen = static_cast<U32>(rpath.size());

		for (auto *res : _resources)
		{
			if (res->_isDeleted)
				continue;
			cvt.UTF8(res->Path.startWith(rpath) ? res->Path.substr(rplen) : res->Path);
			Tools::UUID2String(res->UID, uidbuf);
			auto *entry = out.NewElement("Resource");
			entry->SetAttribute("Name", res->Name.c_str());
			entry->SetAttribute("Type", res->Type);
			entry->SetAttribute("Update", res->_fileTimeStamp);
			entry->SetAttribute("UID", uidbuf);
			entry->SetAttribute("Size", (I64)res->_resourceSize);
			entry->SetText(cvt.c_str());
			rm->InsertEndChild(entry);
		}

		out.InsertFirstChild(rm);
		XMLPrinter prn;
		out.Print(&prn);

		WSTR manifestFileName = rpath + MANIFESTFILENAME;

		Tools::SaveFile(prn.CStrSize() - 1, prn.CStr(), manifestFileName);
		printf("-------------- xml ------------------\n%s\n-------------------------------\n", prn.CStr());
	}
};

// ============================================================================ ResourceManager ===

ResourceManager::ResourceManager()                 { _data = make_new<InternalData>(); }
ResourceManager::~ResourceManager()                { make_delete<InternalData>(_data); }
ResourceManager * ResourceManager::Create()        { return make_new<ResourceManager>(); }
void ResourceManager::Destroy(ResourceManager *rm) { make_delete<ResourceManager>(rm); }

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
	ResourceBase *ret = nullptr;
	for (auto &res : _data->_resources)
	{
		// if we have correct name and typeid, then we have found
		if (res->Name == resName && res->Type == typeId && typeId != ResourceBase::UNKNOWN)
		{
			ret = res;
			break;
		}

		// if we have:
		// - right name
		// - we don't know type when we ask (typeId == UNKNOWN) or resource type is UNKNOWN (ret->Type == UNKNOWN)
		// - we don't have other candidates or we don't know resource type for previous candidate
		if (res->Name == resName &&
			(typeId == ResourceBase::UNKNOWN || res->Type == ResourceBase::UNKNOWN) &&
			(ret == nullptr || (ret != nullptr && ret->Type == ResourceBase::UNKNOWN)))
		{
			ret = res; // we have another candidate
		}
	}

	if (!ret)
	{
		ret = make_new<ResourceBase>();
		ret->Name = resName;
		ret->Type = typeId;
		_data->AddResource(ret);
	}


	// if we don't have type set, when set one.
	if (ret->Type == ResourceBase::UNKNOWN)
	{
		ret->Type = typeId != ResourceBase::UNKNOWN ? typeId : RawData::GetTypeId();
	}

	CreateResourceImplementation(ret);

	return ret;
}

ResourceBase * ResourceManager::Get(const UUID & resUID)
{
	ResourceBase * ret = nullptr;
	for (auto &res : _data->_resources)
	{
		if (res->UID == resUID)
		{
			ret = res;
			break;
		}
	}

	// no resource... create one... well it is "generic", so it is more/less ony name and file.
	if (!ret)
	{
		auto res = make_new<ResourceBase>();
		res->UID = resUID;
		_data->AddResource(res);
		ret = res;
	}

	return ret;
}


// few more "one liners"
void ResourceManager::LoadSync() { _data->LoadEverything(); }
ResourceBase *ResourceManager::Add(CWSTR path) { return _data->AddResource(path); }
void ResourceManager::CreateResourceImplementation(ResourceBase *res) { _data->CreateResourceImplementation(res); }
void ResourceManager::AddDir(CWSTR path) { _data->AddDirToMonitor(path, 0); }
void ResourceManager::RootDir(CWSTR path) { _data->RootDir(path); }
void ResourceManager::StartDirectoryMonitor() { _data->StartMonitoring(); }
void ResourceManager::StopDirectoryMonitor() { _data->StopMonitoring(); }


// ============================================================================ ResourceManagerModule ===

ResourceManager *globalResourceManager = nullptr;
void ResourceManagerModule::Update(float dt)
{

}

void ResourceManagerModule::Initialize()
{
	globalResourceManager = ResourceManager::Create();
	globalResourceManager->StartDirectoryMonitor();
}

void ResourceManagerModule::Finalize()
{
	globalResourceManager->StopDirectoryMonitor();
	ResourceManager::Destroy(globalResourceManager);
	globalResourceManager = nullptr;
}

void ResourceManagerModule::SendMsg(Message *msg)
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

ResourceManager *ResourceManagerModule::GetResourceManager()
{
	return globalResourceManager;
}

NAMESPACE_CORE_END