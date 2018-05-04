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
	basic_array<ResourceBase *, Allocator> _resources;
	DirectoryChangeNotifier _monitoredDirs;

	InternalData() {};

	virtual ~InternalData()
	{
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
		_monitoredDirs.AddDir(std::move(sPath), type); 
	}

	
	void Load(ResourceBase *res)
	{
		SIZE_T size = 0;
		BYTE *data = nullptr;
		data = Tools::LoadFile(&size, res->Path, res->GetMemoryAllocator());
		res->ResourceLoad(data, size);
	}	

	void StartMonitoring()
	{
		_monitoredDirs.Start();
	}

	void StopMonitoring()
	{
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
	ResourceBase *ret = nullptr;
	for (auto &res : _data->_resources)
	{
		// if we have correct name and typeid, then we have found
		if (res->Name == resName && res->Type == typeId)
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
	// if resource is created... return it

	if (ret->_resourceImplementation)
		return ret;


	// no resource... create one... well only generic resource with name, no file, no data, no type
	if (!ret)
	{
		ret = make_new<ResourceBase>();
		ret->Name = resName;
		ret->Type = typeId;
		_data->AddResource(ret);
	}


	// if we know type, because we asked: typeId != UNKNOWN and resource don't have it set: ret->Type == UNKNOWN)
	if (ret->Type == ResourceBase::UNKNOWN && typeId != ResourceBase::UNKNOWN)
	{
		// TODO: should we let know ResourceManager, that we have to update resource manifest?
		ret->Type = typeId; // we set resource type
	}


	// if we know resource type, create resource
	if (ret->Type != ResourceBase::UNKNOWN)
	{
		// we need to find correct "create" function ...
		for (auto f = ResourceFactoryChain::First; f; ++f)
		{
			if (f->TypeId == ret->Type)
			{
				// ... an create it
				ret->_resourceImplementation = f->Create();
				return ret;
			}
		}
	}

	return ret;
}

ResourceBase * ResourceManager::Get(const UUID & resUID)
{
	for (auto &res : _data->_resources)
	{
		if (res->UID == resUID)
			return res;
	}

	// no resource... create one... well it is "generic", so it is more/less ony name and file.
	auto res = make_new<ResourceBase>();
	_data->AddResource(res);

	return res;
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


// few "one liners"
void ResourceManager::Add(CWSTR path) { _data->AddResource(path); }
void ResourceManager::AddDir(CWSTR path) { _data->AddDirToMonitor(path, 0); }

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