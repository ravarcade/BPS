#include "stdafx.h"

/*
 ResourceBase:
 1. Type = UNKNOWN               : Added to resource manager but not recognized yet.
 2. Type = ResRawData::GetTypeId()  : All resources not matching any resonable resource type.
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
namespace BAMS {
namespace CORE {

using tinyxml2::XMLPrinter;
using tinyxml2::XMLDocument;
const wchar_t *MANIFESTFILENAME = L"resource_manifest.xml";

// ============================================================================ ResourceBase ===

void ResourceBase::_CreateResourceImplementation()
{
	// create resource
	globalResourceManager->CreateResourceImplementation(this);
}

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

void ResourceBase::Init(CSTR name)
{
	_resourceData = nullptr;
	_resourceSize = 0;
	_isLoaded = false;
	_refCounter = 0;
	// we do nothing with Path, by default it is empty
	Name = name;
	Tools::CreateUUID(UID);
}

// ============================================================================ ResourceFactoryChain ===

ResourceFactoryChain * ResourceFactoryChain::First = nullptr;

// ============================================================================ ResourceManager::InternalData ===


struct ResourceManager::InternalData : public Allocators::Ext<>
{
	basic_array<ResourceBase *, Allocators::default> _resources;
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

	void MakeNameUniq(ResourceBase *res)
	{
		bool repeat = true;
		while (repeat)
		{
			repeat = false;
			for (auto &r : _resources)
			{
				if (r != res && r->Name == res->Name)
				{
					repeat = true;
				}
			}

			if (repeat)
			{
				// number at end of file name
				auto e = res->Name.end();
				auto f = res->Name.begin();
				int num = 2;
				while (f < e && std::isdigit(e[-1]))
					--e;
				if (e != res->Name.end())
				{
					auto ee = res->Name.end();
					auto ef = e;
					num = 0;
					while (ef < ee)
					{
						num = num * 10 + (*ef) - '0';
						++ef;
					}
					++num;
				}
				res->Name.resize(static_cast<U32>(e - f));
				res->Name += num;
			}
		}
	}

	ResourceBase *AddResource(ResourceBase *res)
	{
		res->ResourceLoad(nullptr, 0);
		MakeNameUniq(res);
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
		MakeNameUniq(res);
		_resources.push_back(res);
		return res;
	}

	ResourceBase *AddResource(CSTR resName, U32 type)
	{
		auto res = make_new<ResourceBase>();
		res->Init(resName);
		res->Type = type;
		MakeNameUniq(res);
		_resources.push_back(res);
		CreateResourceImplementation(res);
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
			if ((*f)->Path.startsWith(filename) && ((*f)->Path.size() == filename.size() || (*f)->Path[filename.size()] == Tools::directorySeparatorChar))
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
				if (res->_resourceTimestamp != resTime ||
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
		if (_rootDir.startsWith(filename) && (_rootDir.size() == fnlen || _rootDir[fnlen] == Tools::directorySeparatorChar))
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
		CEngine::SendMsg(RESOURCE_MANIFEST_PARSED);
		ProcessUnknownResources();
	}

	void CreateResourceImplementation(ResourceBase *res)
	{
		if (res->_resourceImplementation)
		{
			if (res->_resourceImplementation->GetFactory()->TypeId == res->Type)
				return; // do not recreate resource if it exist and is same type

			// delete existing resource
			res->_resourceImplementation->GetFactory()->Destroy(res->_resourceImplementation);
			res->_resourceImplementation = nullptr;
		}

		// we must have resource implemented
		while (res->_resourceImplementation == nullptr)
		{
			// we must have res type set
			if (res->Type == RESID_UNKNOWN)
				res->Type = ResRawData::GetTypeId();

			// we need to find correct "create" function ...
			for (auto f = ResourceFactoryChain::First; f; f = f->Next)
			{
				if (f->TypeId == res->Type)
				{
					// ... an create it
					res->_resourceImplementation = f->Create(res);
					res->_isLoadable = f->IsLoadable();
					break;
				}
			}

			// ... and if we still don't know what it is... it must be ResRawData.
			if (res->_resourceImplementation == nullptr)
				res->Type = ResRawData::GetTypeId();
		}
	}

	void SetResourceType(ResourceBase *res, U32 resTypeId)
	{
		if (res->_resourceImplementation)
		{
			if (res->Type == resTypeId)
				return;

			// delete existing resource
			res->_resourceImplementation->GetFactory()->Destroy(res->_resourceImplementation);
			res->_resourceImplementation = nullptr;
		}

		res->Type = resTypeId;

		// create only if it is used
		if (res->_refCounter)
			CreateResourceImplementation(res);
	}

	void Load(ResourceBase *res)
	{
		if (res->_isDeleted || (res->_isModified && res->_waitWithUpdate > clock::now()))
			return;

		SIZE_T size = 0;
		BYTE *data = nullptr;

		//if (res->Type == RESID_UNKNOWN) 
		//	res->_resourceImplementation->Release(res);

		// we are loading resource to ram, so it is in use and resource implementation must be created
		CreateResourceImplementation(res);

		if (res->_isLoadable) {
			data = Tools::LoadFile(&size, &res->_resourceTimestamp, res->Path, res->GetMemoryAllocator());
		}
		else {
			Tools::InfoFile(&size, &res->_resourceTimestamp, res->Path);
		}

		// mark resource as deleted if file not exist and can't be loaded
		if (data == nullptr && size == 0)
			res->_isDeleted = !Tools::Exist(res->Path);

		if (!res->_isDeleted)
		{
			res->ResourceLoad(data, size);
			if (!res->_isLoadable)
				res->_isLoaded = true;

			res->_isModified = false;
		}
	}

	WSTR fileName, fileNameRenameTo;
	static void Worker(InternalData *rm)
	{
		while (!rm->_killWorkerFlag)
		{
			// this will wait 1 second for new events to process
			SleepEx(1000, TRUE); // we do nothing... everything is done in os
			bool resUpdated = false;
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
							resUpdated = true;
						}
					}
					break;

				case FILE_ACTION_REMOVED:
					rm->RemoveResource(rm->fileName.c_str());
					wprintf(L"Delete: [%s] \n", fn.c_str());
					resUpdated = true;
					break;

				case FILE_ACTION_MODIFIED:
					if (!Tools::IsDirectory(rm->fileName))
					{
						rm->UpdateResource(fn);
						wprintf(L"Modify: [%s]\n", fn.c_str());
						resUpdated = true;
					}
					break;

				case FILE_ACTION_RENAMED_NEW_NAME:
					rm->RenameResource(fn, frn);
					wprintf(L"Rename: [%s] -> [%s]\n", fn.c_str(), frn.c_str());
					break;

				case FILE_ACTION_RENAMED_OLD_NAME:
				default:
					wprintf(L"!!! Default: error.\n");
					break;
				}
			}
			rm->ProcessResources();
			wprintf(L"Still alive... \n");
		}
	}

	void ProcessResource(ResourceBase *res, clock::time_point &now, bool &allLoaded)
	{
		if (res->Type == 0)
			TRACE("t=0\n");

		if (res->Type == RESID_UNKNOWN)
			IdentifyResourceType(res);

		if (res->_isLoaded && (res->_isDeleted || (res->_isModified && res->_waitWithUpdate > now)))
		{
			res->_updateNotifier.Notify();
			res->GetImplementation()->Release(res);
		}

		if (res->_isDeleted)
			return;

		if (!res->_isLoaded && res->_refCounter && (!res->_isModified || res->_waitWithUpdate < now))
		{
			wprintf(L"Loading: %s\n", res->Path.c_str());
			Load(res);
			if (res->_isLoaded)
			{
				res->Update();
			}
		}

		if (res->_refCounter && !res->_isLoaded && res->Path.size())
		{
			allLoaded = false;
		}
	}

	bool ProcessResources(ResourceBase *res = nullptr)
	{
		bool allLoaded = true;
		std::lock_guard<std::mutex> lck(_fileLoadingMutex);
		auto now = clock::now();

		if (res)
		{
			ProcessResource(res, now, allLoaded);
			if (!res->_isLoadable)
				res->_isLoaded = true;
		}
		else
		{
			for (auto &res : _resources)
			{
				ProcessResource(res, now, allLoaded);
			}
		}

		return allLoaded;
	}

	void LoadEverything(ResourceBase *res)
	{
		if (res)
			res->_isDeleted = false;
		bool allLoaded = ProcessResources(res);
		while (!_killWorkerFlag && !allLoaded)
		{
			allLoaded = ProcessResources(res);
			SleepEx(100, TRUE);
		}
	}

	void RefreshResorceFileInfo(ResourceBase *res)
	{
		res->_isDeleted = !Tools::InfoFile(&res->_resourceSize, &res->_resourceTimestamp, res->Path);
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

	void IdentifyResourceType(ResourceBase *res)
	{
		ResShader::IdentifyResourceType(res);
		ResImportModel::IdentifyResourceType(res);
		if (res->Type == RESID_UNKNOWN)
			res->Type = RESID_RAWDATA;
	}

	void ProcessUnknownResources()
	{
		for (auto *res : _resources)
		{
			if (res->_isDeleted)
				continue;
			if (res->Type == RESID_UNKNOWN)
			{
				IdentifyResourceType(res);
				TRACEW("RESID_UNKNOWN: " << res->Name.c_str() << "\n");
			}
		}
	}

	void LoadManifest()
	{
		WSTR rpath = _rootDir + Tools::wDirectorySeparator;
		_manifestFileName = rpath + MANIFESTFILENAME;
		SIZE_T manifestSize = 0;
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
				auto res = make_new<ResourceBase>();

				auto innerXML = r->GetText();
				res->XML = innerXML? innerXML : "";
				res->Path.UTF8(r->Attribute("Path"));
				AbsolutePath(res->Path);
				res->Name = r->Attribute("Name");
				SetResourceType(res, r->UnsignedAttribute("Type", RESID_UNKNOWN));
				const char *uid = r->Attribute("UID");
				Tools::String2UUID(res->UID, uid);
				res->_resourceTimestamp = r->Int64Attribute("Update", 0);
				res->_resourceSize = r->Int64Attribute("Size", 0);
				_resources.push_back(res);
			}
			GetMemoryAllocator()->deallocate(manifest);			
		}
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
			cvt.UTF8(res->Path.startsWith(rpath) ? res->Path.substr(rplen) : res->Path);
			Tools::UUID2String(res->UID, uidbuf);
			auto *entry = out.NewElement("Resource");
			entry->SetAttribute("Name", res->Name.c_str());
			entry->SetAttribute("Type", res->Type);
			entry->SetAttribute("Update", res->_resourceTimestamp);
			entry->SetAttribute("UID", uidbuf);
			entry->SetAttribute("Size", (I64)res->_resourceSize);
			entry->SetAttribute("Path", cvt.c_str());
			entry->SetText(res->XML.c_str());
			rm->InsertEndChild(entry);
		}

		out.InsertFirstChild(rm);
		XMLPrinter prn;
		out.Print(&prn);

		WSTR manifestFileName = rpath + MANIFESTFILENAME;

		Tools::SaveFile(manifestFileName, prn.CStr(), prn.CStrSize() - 1);
		printf("-------------- xml ------------------\n%s\n-------------------------------\n", prn.CStr());
	}

	void AbsolutePath(WSTR & filename, const WSTR *root = nullptr)
	{
		if (!filename.size())
			return;

		if (!root)
			root = &_rootDir;

		if (filename.size() < 3 ||
			!(
			((filename[0] >= 'a' && filename[0] <= 'z') || (filename[0] >= 'A' && filename[0] <= 'Z')) &&
				filename[1] == ':' &&
				(filename[2] == '\\' || filename[2] == '/')
				)
			)
		{
			if (filename[0] == '/' || filename[0] == '\\')
				filename = (*root) + filename;
			else
				filename = (*root) + Tools::wDirectorySeparator + filename;
		}

		Tools::NormalizePath(filename);
	}

	void RelativePath(WSTR &filename, const WSTR *root)
	{
		if (!root)
			root = &_rootDir;

		if (filename.size() > root->size() && filename.startsWith(*root) && (filename[root->size()] == '\\' || filename[root->size()] == '/'))
		{
			filename = filename.substr(static_cast<I32>(root->size()) + 1); // +1 for directory separator
		}
	}

	WSTR GetDirPath(const WString &filename)
	{
		WSTR ret(filename);

		U32 newSize = filename.size();
		for (U32 i = 0; i < filename.size(); ++i)
		{
			if (filename[i] == '\\' || filename[i] == '/')
				newSize = i;
		}
		ret.resize(newSize);
		return std::move(ret);
	}

};

// ============================================================================ ResourceManager ===

ResourceManager::ResourceManager()                 { _data = make_new<InternalData>(); }
ResourceManager::~ResourceManager()                { make_delete<InternalData>(_data); }
ResourceManager * ResourceManager::Create()        { return make_new<ResourceManager>(); }
void ResourceManager::Destroy(ResourceManager *rm) { make_delete<ResourceManager>(rm); }

void ResourceManager::Filter(void(*callback)(ResourceBase *, void *), void *localData, CSTR  _namePattern, U32 typeId)
{
	STR namePattern(_namePattern ? _namePattern : "");

	for (auto &res : _data->_resources)
	{
		if ((typeId == RESID_UNKNOWN || typeId == res->Type) &&
			(namePattern.size() == 0 || res->Name.wildcard(namePattern)))
		{
			callback(res, localData);
		}
	}
}

ResourceBase *ResourceManager::GetByFilename(const WSTR &filename, U32 typeId)
{
	ResourceBase *ret = nullptr;

	for (ResourceBase **res = nullptr; _data->FindResourceByFilename(res, filename); )
	{
		if (!(*res)->_isDeleted &&
			(typeId == RESID_UNKNOWN || (*res)->Type == RESID_UNKNOWN || (*res)->Type == typeId))
		{
			ret = *res;
			break;
		}
	}

	if (!ret)
	{
		ret = make_new<ResourceBase>();
		ret->Init(filename.c_str());
		ret->Type = typeId;
		_data->AddResource(ret);
	}

	// if we don't have type set, when set one.
	if (ret->Type == RESID_UNKNOWN)
	{
		ret->Type = typeId;
	}

	CreateResourceImplementation(ret);

	return ret;
}

ResourceBase * ResourceManager::Get(const STR & resName, U32 typeId)
{
	ResourceBase *ret = nullptr;
	for (auto &res : _data->_resources)
	{
		// if we have correct name and typeid, then we have found
		if (res->Name == resName && res->Type == typeId && typeId != RESID_UNKNOWN)
		{
			ret = res;
			break;
		}

		// if we have:
		// - right name
		// - we don't know type when we ask (typeId == UNKNOWN) or resource type is UNKNOWN (ret->Type == UNKNOWN)
		// - we don't have other candidates or we don't know resource type for previous candidate
		if (res->Name == resName &&
			(typeId == RESID_UNKNOWN || typeId == RESID_RAWDATA || res->Type == RESID_UNKNOWN || res->Type == RESID_RAWDATA) &&
			(ret == nullptr || (ret != nullptr && (ret->Type == RESID_UNKNOWN || ret->Type == RESID_RAWDATA))))
		{
			ret = res; // we have another candidate
		}
	}

	if (!ret)
	{
		ret = make_new<ResourceBase>();
		ret->Init(resName.c_str());
		ret->Type = typeId;
		_data->AddResource(ret);
	}

	// if we don't have type set, when set one.
	if (ret->Type == RESID_UNKNOWN ||
		(ret->Type == RESID_RAWDATA && typeId != RESID_UNKNOWN))
	{
		ret->Type = typeId;
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
void ResourceManager::LoadSync(ResourceBase *res) { _data->LoadEverything(res); }
void ResourceManager::RefreshResorceFileInfo(ResourceBase *res) { _data->RefreshResorceFileInfo(res); }
ResourceBase *ResourceManager::Add(CWSTR path) { return _data->AddResource(path); }
ResourceBase *ResourceManager::Add(CSTR resName, U32 type) { return _data->AddResource(resName, type); }
void ResourceManager::CreateResourceImplementation(ResourceBase *res) { _data->CreateResourceImplementation(res); }
void ResourceManager::AddDir(CWSTR path) { _data->AddDirToMonitor(path, 0); }
void ResourceManager::RootDir(CWSTR path) { _data->RootDir(path); }
void ResourceManager::StartDirectoryMonitor() { _data->StartMonitoring(); }
void ResourceManager::StopDirectoryMonitor() { _data->StopMonitoring(); }

void ResourceManager::AbsolutePath(WSTR & filename, const WSTR *root) { _data->AbsolutePath(filename, root); }
void ResourceManager::RelativePath(WSTR & filename, const WSTR *root) { _data->RelativePath(filename, root); }
WSTR ResourceManager::GetDirPath(const WString & filename) { return std::move(_data->GetDirPath(filename)); }

// ============================================================================ ResourceManagerModule ===

ResourceManager *globalResourceManager = nullptr;
void ResourceManagerModule::Update(float dt)
{
	//
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
	case RESOURCE_ADD_FILE:
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

}; // CORE namespace
}; // BAMS namespace