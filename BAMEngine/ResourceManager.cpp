#include "stdafx.h"

/*
 ResBase:
 1. Type = UNKNOWN               : Added to resource manager but not recognized yet.
 2. Type = ResRawData::GetTypeId()  : All resources not matching any resonable resource type.
 3. Type = RealResourceType::GetTypeId()

 ResBase states:
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

 [6] Recognized, no need to load, used

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


// ============================================================================ ResourceFactoryChain ===

ResourceFactoryChain * ResourceFactoryChain::First = nullptr;

// ============================================================================ ResourceManager::InternalData ===


struct ResourceManager::InternalData : public Allocators::Ext<>
{
	CStringHastable<ResBase> _resources;
//	basic_array<ResBase *, Allocators::default> _resources;
	
	DirectoryChangeNotifier _monitoredDirs;
	std::thread *_worker;
	bool _killWorkerFlag;
	std::mutex _fileLoadingMutex;
	WSTR _rootDir;
	WSTR _manifestFileName;
	XMLDocument _manifest;

	InternalData() :
		_worker(nullptr),
		_killWorkerFlag(false)
	{};

	virtual ~InternalData()
	{
		StopMonitoring();
		SaveManifest();
		//for (auto *pRes : _resources)
		//{
		//	make_delete<ResBase>(pRes);
		//}
		_resources.clear();
	}

	inline ResBase *NewResBase(CSTR name, tinyxml2::XMLElement * xml = nullptr) 
	{ 
		if (!xml)
			xml = NewXMLElement();
		return _resources.add(name, name, xml);
	}

	inline ResBase *NewResBase(CWSTR path) // creates uniq resource
	{
		CWSTR e = path;
		CWSTR lastDot = nullptr;
		CWSTR lastSlash = nullptr;
		
		while (*e) 
		{
			if (*e == '.')
				lastDot = e;
			if (*e == '\\' || *e == '/')
				lastSlash = e + 1;
			++e;
		}
		if (lastDot == nullptr)
			lastDot = e;
		if (lastSlash == nullptr)
			lastSlash = path;

		STR possibleName(WString(lastSlash, lastDot));
		MakeNameUniq(possibleName);
		auto ret = NewResBase(possibleName.c_str());
		WSTR p = path;
		Tools::NormalizePath(p);
		AbsolutePath(p);
		ret->Path = p;
		return ret;
	}

	void MakeNameUniq(STR &name)
	{
		while (_resources.findIdx(name.c_str()))
		{
			// number at end of file name
			auto e = name.end();
			auto f = name.begin();
			int num = 2;
			while (f < e && std::isdigit(e[-1]))
				--e;
			if (e != name.end())
			{
				auto ee = name.end();
				auto ef = e;
				num = 0;
				while (ef < ee)
				{
					num = num * 10 + (*ef) - '0';
					++ef;
				}
				++num;
			}
			name.resize(static_cast<U32>(e - f));
			name += num;
		}		
	}

	bool IsUniqFile(CWSTR path)
	{
		return FilterByFilename(ToBasicString(path)) == nullptr;
	}

	// ======================================================================== AddResource ====
	ResBase *AddResource(CSTR resName, U32 type)
	{
		auto res = NewResBase(resName);
		Tools::CreateUUID(res->UID);
		res->Type = type;
		res->_resTimestamp = std::time(nullptr);
		return res;
	}

	ResBase *AddResource(CWSTR resFilename, U32 type)
	{
		auto res = NewResBase(resFilename);
		Tools::CreateUUID(res->UID);
		res->Type = type;
		res->_resTimestamp = std::time(nullptr);
		return res;
	}

	ResBase *AddResource(const UUID &resUID, U32 type)
	{
		auto res = NewResBase("_noname_");
		res->UID = resUID;
		res->Type = type;
		res->_resTimestamp = std::time(nullptr);
		return res;
	}

	// ======================================================================== FindOrCreate ====
	inline ResBase *Create(CSTR name, U32 type)
	{
		ResBase *res = _resources.find(name);
		if (res) {
			STR uniqName = name;
			MakeNameUniq(uniqName);
			res = NewResBase(uniqName.c_str());
		}
		else {
			res = NewResBase(name);
		}
		Tools::CreateUUID(res->UID);
		res->Type = type;
		res->_resTimestamp = std::time(nullptr);
		return res;
	}


	inline ResBase *FindOrCreate(CSTR name, U32 type)
	{
		auto ret = _resources.find(name);
		if (!ret && type != RESID_FORBIDEN)
			ret = AddResource(name, type);

		return ret;
	}

	ResBase *FindOrCreate(const UUID &resUID, U32 type)
	{
		for (auto res : _resources)
		{
			if (res->UID == resUID)
				return res;
		}

		return type != RESID_FORBIDEN ? AddResource(resUID, type) : nullptr;
	}

	ResBase *FindOrCreate(CWSTR filename, U32 type)
	{
		auto &fn = ToBasicString(filename);
		for (auto res : _resources)
		{
			if (res->Path == fn)
				return res;
		}

		return type != RESID_FORBIDEN ? AddResource(filename, type) : nullptr;
	}

	// ========================================================================
	ResBase *FilterByFilename(const WString &filename)
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

	bool FindResourceByFilename(ResBase **&out, const WSTR &filename)
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
		auto waitTime = clock::now() + ResBase::defaultDelay;
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
				if (res->_resTimestamp != resTime ||
					res->_resSize != resSize)
				{
					// modified
					res->_waitWithUpdateNotification = waitTime;
					res->_isModified = true;
				}
			}
		}
	}

	static void SearchForFilesCallback(WSTR &filename, U64 fileSize, time_t timestamp, void *ctrl)
	{
		auto rm = reinterpret_cast<ResourceManager::InternalData *>(ctrl);
		if (!(filename == rm->_manifestFileName)) 
		{
			auto fn = filename.c_str();
			if (rm->IsUniqFile(fn)) {
				auto res = rm->AddResource(fn, RESID_UNKNOWN);
				res->_resTimestamp = timestamp;
				res->_resSize = fileSize;
			}

		}
	}

	void ScanRootdir()
	{
		Tools::SearchForFiles(_rootDir, &SearchForFilesCallback, this);
	}

	void UpdateResource(const WSTR &filename)
	{
		auto waitTime = clock::now() + ResBase::defaultDelay;
		for (ResBase **res = nullptr; FindResourceByFilename(res, filename); )
		{
			if (!(*res)->_isDeleted)
			{
				(*res)->_waitWithUpdateNotification = waitTime;
				(*res)->_isModified = true;
			}
		}
	}

	void RemoveResource(const WSTR &filename)
	{
		for (ResBase **res = nullptr; FindResourceByFilename(res, filename); )
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

		for (ResBase **res = nullptr; FindResourceByFilename(res, filename); )
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
		dbg::Log(L"Monitorning dir: %s\r\n", sPath.c_str());
		_monitoredDirs.AddDir(std::move(sPath), type);
	}

	void RootDir(CWSTR path)
	{
		_rootDir = path;
		Tools::NormalizePath(_rootDir);
		LoadManifest();
		ScanRootdir();
		CEngine::SendMsg(RESOURCE_MANIFEST_PARSED);
		ProcessUnknownResources();
		AddDirToMonitor(path, 0);
	}

	IResourceFactory *FindResourceFactory(ResBase *res)
	{
		for (auto f = ResourceFactoryChain::First; f; f = f->Next)
		{
			if (f->TypeId == res->Type)
				return f;
		}
		return nullptr;
	}

	void CreateResourceImplementation(ResBase *res)
	{
		isResourceProcessingRequired = true;
		if (res->_pResImp)
		{
			if (res->_pResImp->GetFactory()->TypeId == res->Type)
				return; // do not recreate resource if it exist and is same type

			// delete existing resource
			res->_pResImp->GetFactory()->Destroy(res->_pResImp);
			res->_pResImp = nullptr;
		}

		// we must have resource implemented
		while (res->_pResImp == nullptr)
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
					res->_pResImp = f->Create(res);
					res->_isLoadable = f->IsLoadable();
					break;
				}
			}

			// ... and if we still don't know what it is... it must be ResRawData.
			if (res->_pResImp == nullptr)
				res->Type = ResRawData::GetTypeId();
		}
	}

	void SetResourceType(ResBase *res, U32 resTypeId)
	{
		if (res->_pResImp)
		{
			if (res->Type == resTypeId)
				return;

			// delete existing resource
			res->_pResImp->GetFactory()->Destroy(res->_pResImp);
			res->_pResImp = nullptr;
		}

		res->Type = resTypeId;

		// create only if it is used
		if (res->_refCounter)
			CreateResourceImplementation(res);
		else {
			if (auto f = FindResourceFactory(res))
			{
				res->_isLoadable = f->IsLoadable();
			}
		}
	}

	void Load(ResBase *res)
	{
		if (res->_isDeleted || !res->Path.size())
			return;
		//if (res->_isDeleted ||
		//	!res->Path.size() ||
		//	(res->_isModified && res->_waitWithUpdateNotification > clock::now())) // if file modification is detected, wait some time before load it
		//	return;

		SIZE_T size = 0;
		BYTE *data = nullptr;

		// we are loading resource to ram, so it is in use and resource implementation must be created
		CreateResourceImplementation(res);

		if (res->_isLoadFromDiskAllowed) {
			wprintf(L"Loading: %s\n", res->Path.c_str());
			data = Tools::LoadFile(&size, &res->_resTimestamp, res->Path, res->GetMemoryAllocator());
		}
		else {
			if (res->_isModified) {
				wprintf(L"Checking: %s\n", res->Path.c_str());
				Tools::InfoFile(&size, &res->_resTimestamp, res->Path);
			}
			else {
				size = res->_resSize;
			}
		}

		// mark resource as deleted if file not exist and can't be loaded
		if (data == nullptr && size == 0)
			res->_isDeleted = !Tools::Exist(res->Path);

		if (!res->_isDeleted)
		{
			res->_resData = data;
			res->_resSize = size;
			res->_isLoaded = true;
			res->_isModified = false;
			res->_isLoadFromDiskAllowed = false; // don't load again
		}
	}

	WSTR fileName, fileNameRenameTo;
	bool isResourceProcessingRequired;

	static void Worker(InternalData *rm)
	{
		
		while (!rm->_killWorkerFlag)
		{
			// this will wait 1 second for new events to process
			SleepEx(250, TRUE); // we do nothing... everything is done in os
			rm->_monitoredDirs.Start();
			bool isResUpdated = false;
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
						if (rm->AddResource(rm->fileName.c_str(), RESID_UNKNOWN))
						{
							wprintf(L"Add: [%s] \n", fn.c_str());
							isResUpdated = true;
						}
					}
					break;

				case FILE_ACTION_REMOVED:
					rm->RemoveResource(rm->fileName.c_str());
					wprintf(L"Delete: [%s] \n", fn.c_str());
					isResUpdated = true;
					break;

				case FILE_ACTION_MODIFIED:
					if (!Tools::IsDirectory(rm->fileName))
					{
						rm->UpdateResource(fn);
						wprintf(L"Modify: [%s]\n", fn.c_str());
						isResUpdated = true;
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
			rm->isResourceProcessingRequired |= isResUpdated;
			if (rm->isResourceProcessingRequired) 
			{
				rm->ProcessResources();
			}
		}
		rm->_monitoredDirs.Stop();
	}

	void ProcessResource(ResBase *res, clock::time_point &now, bool &allLoaded)
	{
		if (res->Type == 0)
			TRACE("t=0\n");

		if (res->Type == RESID_UNKNOWN)
			IdentifyResourceType(res);

		if (res->_isModified && res->_waitWithUpdateNotification >= now) // if we deal with modified file, we will exit here
		{
			isResourceProcessingRequired = true; // this will inform resource monitor, that we did not finish resource processing
			return;
		}

		if (res->_isLoaded && (res->_isDeleted || res->_isModified))
		{
			res->_updateNotifier.Notify();
			res->GetImplementation()->Release(res);			
			res->_isLoadFromDiskAllowed |= res->_isModified && res->_isLoadable;
		}

		if (res->_isDeleted || !res->_refCounter)
			return;

		//if (!res->_isLoaded && res->_refCounter && (!res->_isModified || res->_waitWithUpdateNotification < now))
		if (!res->_isLoaded || res->_isModified)
		{
			Load(res);
			if (res->_isLoaded)
			{
				res->Update();
			}
		}

		if (!res->_isLoaded && res->Path.size())
		{
			allLoaded = false;
		}
	}

	bool ProcessResources(ResBase *res = nullptr)
	{
		bool allLoaded = true;
		std::lock_guard<std::mutex> lck(_fileLoadingMutex);
		auto now = clock::now();

		if (res)
		{
			ProcessResource(res, now, allLoaded);
			if (!res->_isLoadFromDiskAllowed) {
				if (!res->_isLoaded)
					TRACE("hmmm? coœ nie dzia³a dobrze.");
				res->_isLoaded = true;
			}
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

	void LoadEverything(ResBase *res)
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

	void RefreshResorceFileInfo(ResBase *res)
	{
		res->_isDeleted = !Tools::InfoFile(&res->_resSize, &res->_resTimestamp, res->Path);
	}

	void StartMonitoring()
	{
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
	}

	void IdentifyResourceType(ResBase *res)
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

	tinyxml2::XMLElement *NewXMLElement(const char* name = "") { return _manifest.NewElement(name); }

	void LoadManifest()
	{
		WSTR rpath = _rootDir + Tools::wDirectorySeparator;
		_manifestFileName = rpath + MANIFESTFILENAME;
		SIZE_T manifestSize = 0;
		STR fn;

		char *manifest = reinterpret_cast<char *>(Tools::LoadFile(&manifestSize, nullptr, _manifestFileName, GetMemoryAllocator()));
		if (manifest)
		{
			_manifest.Parse(manifest, manifestSize);

			auto root = _manifest.FirstChildElement("ResourcesManifest");
			for (auto r = root->FirstChildElement("Resource"); r; r = r->NextSiblingElement())
			{				
				auto res = NewResBase(r->Attribute("Name"), r);
				res->Path.UTF8(r->Attribute("Path"));
				AbsolutePath(res->Path);
				SetResourceType(res, r->UnsignedAttribute("Type", RESID_UNKNOWN));
				const char *uid = r->Attribute("UID");
				Tools::String2UUID(res->UID, uid);
				res->_resTimestamp = r->Int64Attribute("Update", 0);
				res->_resSize = r->Int64Attribute("Size", 0);
			}
			GetMemoryAllocator()->deallocate(manifest);			
		}
	}

	void SaveManifest()
	{
		XMLDocument out;
		auto *rm = out.NewElement("ResourcesManifest");
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
			entry->SetAttribute("Update", res->_resTimestamp);
			entry->SetAttribute("UID", uidbuf);
			entry->SetAttribute("Size", (I64)res->_resSize);
			entry->SetAttribute("Path", cvt.c_str());

			if (res->XML && res->XML->FirstChild())
			{
				for (auto child = res->XML->FirstChild(); child; child = child->NextSibling())
				{
					auto clone = child->DeepClone(&out);
					entry->InsertEndChild(clone);
				}
			}
				
			rm->InsertEndChild(entry);
		}

		out.InsertFirstChild(rm);
		XMLPrinter prn;
		out.Print(&prn);

		WSTR manifestFileName = rpath + MANIFESTFILENAME;

		Tools::SaveFile(manifestFileName, prn.CStr(), prn.CStrSize() - 1);
//		printf("-------------- xml ------------------\n%s\n-------------------------------\n", prn.CStr());
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

void ResourceManager::Filter(void(*callback)(ResBase *, void *), void * localData, CSTR namePattern, CWSTR filenamePattern, CSTR filenamePatternUTF8, CWSTR rootPath, U32 typeId, bool caseInsesitive)
{
	WSTR tmp;
	if (filenamePatternUTF8 && !filenamePattern)
	{
		tmp.UTF8(filenamePatternUTF8);
		filenamePattern = tmp.c_str();
	}

	U32 rootPathLen = 0;
	if (rootPath)
	{
		rootPathLen = static_cast<U32>(wcslen(rootPath));
		while (rootPathLen && rootPath[rootPathLen - 1] != Tools::directorySeparatorChar)
			--rootPathLen;
	}

	auto &_namePattern = ToBasicString(namePattern);
	auto &_filenamePattern = ToBasicString(filenamePattern);
	auto &_rootPath = ToBasicString(rootPath, rootPathLen);
	
	for (auto &res : _data->_resources)
	{
		if ((typeId == RESID_UNKNOWN || typeId == res->Type) &&
			(namePattern == nullptr || res->Name.wildcard(_namePattern, caseInsesitive)) &&
			(rootPath == nullptr || res->Path.startsWith(_rootPath, caseInsesitive)))
		{
			if (filenamePattern)
			{
				U32 pos = 0;
				for (U32 i = res->Path.size(); i > 0; --i)
				{
					if (res->Path[i - 1] == Tools::directorySeparatorChar)
					{
						pos = i;
						break;
					}
				}

				if (!res->Path.wildcard(_filenamePattern, caseInsesitive, pos))
					continue;
			}

			callback(res, localData);
		}
	}
}

ResBase * ResourceManager::Create(CSTR name, U32 typeId)                 { return _data->Create(name, typeId); }

ResBase * ResourceManager::FindOrCreate(CSTR name, U32 typeId)           { return _data->FindOrCreate(name, typeId); }
ResBase * ResourceManager::FindOrCreate(CWSTR filename, U32 typeId)      { return _data->FindOrCreate(filename, typeId); }
ResBase * ResourceManager::FindOrCreate(const UUID & resUID, U32 typeId) { return _data->FindOrCreate(resUID, typeId); }

ResBase * ResourceManager::FindExisting(CSTR name, U32 type)
{
	ResBase *res = FindOrCreate(name);
	if (!res || res->Type != type)
	{
		TRACE("ERROR: Resource \"" << name << "\" don't exist or is different type\n");
		return nullptr;
	}
	return res;
}

ResBase * ResourceManager::FindExisting(CWSTR filename, U32 type)
{
	ResBase *res = FindOrCreate(filename);
	if (!res || res->Type != type)
	{
		TRACE("ERROR: Resource for file \"" << filename << "\" don't exist or is different type\n");
		return nullptr;
	}
	return res;
}


ResBase * ResourceManager::FindExisting(const UUID &resUID, U32 type)
{
	ResBase *res = FindOrCreate(resUID);
	if (!res || res->Type != type)
	{
		TRACE("ERROR: Resource for UID don't exist or is different type\n");
		return nullptr;
	}
	return res;
}

// few more "one liners"
void ResourceManager::LoadSync(ResBase *res) { _data->LoadEverything(res); }
void ResourceManager::RefreshResorceFileInfo(ResBase *res) { _data->RefreshResorceFileInfo(res); }
void ResourceManager::CreateResourceImplementation(ResBase *res) { _data->CreateResourceImplementation(res); }
tinyxml2::XMLElement * ResourceManager::NewXMLElement(const char* name) { return _data->NewXMLElement(name); }

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