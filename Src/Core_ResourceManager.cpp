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

	InternalData() :
		_worker(nullptr),
		_killWorkerFlag(false)
	{};

	virtual ~InternalData()
	{
		StopMonitoring();

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

	ResourceBase *AddResource(CWSTR path)
	{
		auto res = make_new<ResourceBase>();
		res->Init(path);
		_resources.push_back(res);
		return res;
	}

	void FilterByFilename(ResourceBase ** resList, U32 * resCount, CWSTR filename, U32 *skip = 0)
	{
		I32 counter = 0;
		I32 max = (resCount && resList) ? *resCount : 0;
		I32 pos = 0;
		WSTR fn(filename);

		for (auto &res : _resources)
		{
			if (res->Path == fn)
			{
				if (&skip && pos < max)
				{
					resList[pos] = res;
					++pos;
				}
				else
				{
					--skip;
				}

				++counter;
			}
		}

		if (resCount)
			*resCount = counter;
	}

	bool FindResourceByFilename(ResourceBase *&out, CWSTR filename)
	{
		WString fn(static_cast<U32>(WString::length(filename)), const_cast<wchar_t*>(filename));

		auto f = _resources.begin();
		auto l = _resources.end();
		if (out)
		{
			while (f != l)
			{
				if (*f == out)
				{
					++f;
					break;
				}
				++f;
			}
		}

		while (f != l)
		{
			if ((*f)->Path == fn)
			{
				out = *f;
				return true;
			}
			++f;
		}

		return false;
	}

	void UpdateResource(CWSTR filename)
	{
		for (ResourceBase *res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!res->_isDeleted)
			{
				res->_waitWithUpdate = clock::now() + ResourceBase::defaultDelay;
				res->_isModified = true;
			}
		}
	}

	void RemoveResource(CWSTR filename)
	{
		for (ResourceBase *res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!res->_isDeleted)
			{
				res->_isDeleted = true;
			}
		}
	}

	void RenameResource(CWSTR filename, CWSTR newfilename)
	{
		for (ResourceBase *res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!res->_isDeleted)
			{
				res->Path = newfilename;
			}
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
			for (auto f = ResourceFactoryChain::First; f; f->Next)
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
			SetResourceType(res, RawData::GetTypeId());

		data = Tools::LoadFile(&size, &res->_fileTimeStamp, res->Path, res->GetMemoryAllocator());
		res->ResourceLoad(data, size);
		res->_isModified = false;
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
					rm->AddResource(rm->fileName.c_str());
					wprintf(L"Add: [%s] \n", fn.c_str());
					++rm->addcount;
					break;
				case FILE_ACTION_REMOVED:
					rm->RemoveResource(rm->fileName.c_str());
					wprintf(L"Delete: [%s] \n", fn.c_str());
					++rm->remcount;
					break;
				case FILE_ACTION_MODIFIED:
					rm->UpdateResource(rm->fileName.c_str());
					wprintf(L"Modify: [%s]\n", fn.c_str());
					++rm->modcount;
					break;
				case FILE_ACTION_RENAMED_NEW_NAME:
					rm->RenameResource(rm->fileName.c_str(), rm->fileNameRenameTo.c_str());
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
		for (auto &res : _resources)
		{
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
};

// ============================================================================ ResourceManager ===

ResourceManager::ResourceManager()
{
	_data = make_new<InternalData>();
}

ResourceManager::~ResourceManager()
{
	_data->StopMonitoring();
	SaveManifest();
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

void ResourceManager::CreateResourceImplementation(ResourceBase *res)
{
	// we must have res type set
	if (res->Type == ResourceBase::UNKNOWN)
		res->Type = RawData::GetTypeId();

	// we must have resource implemented
	if (res->_resourceImplementation == nullptr)
	{
		// we need to find correct "create" function ...
		for (auto f = ResourceFactoryChain::First; f; f->Next)
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


// few "one liners"
void ResourceManager::LoadSync() { _data->LoadEverything(); }

ResourceBase *ResourceManager::Add(CWSTR path) { return _data->AddResource(path); }
void ResourceManager::AddDir(CWSTR path) { _data->AddDirToMonitor(path, 0); }
void ResourceManager::RootDir(CWSTR path) { _data->RootDir(path); }
void ResourceManager::StartDirectoryMonitor() { _data->StartMonitoring(); }
void ResourceManager::StopDirectoryMonitor() { _data->StopMonitoring(); }

void ResourceManager::LoadManifest()
{
}

void ResourceManager::SaveManifest()
{
	XMLDocument out;
	auto *rm = out.NewElement("ResourcesManifest");
	rm->SetAttribute("qty", (int)_data->_resources.size());
	STR cvt;
	char uidbuf[40];
	WSTR rpath = _data->_rootDir + Tools::wDirectorySeparator;
	U32 rplen = static_cast<U32>(rpath.size());

	for (auto *res : _data->_resources)
	{
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
	printf("-------------- xml ------------------\n%s\n-------------------------------\n",prn.CStr());
}

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

ResourceManager *ResourceManagerModule::GetResourceManager()
{
	return globalResourceManager;
}

NAMESPACE_CORE_END