#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

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
	CWSTR pPathEnd = Path._buf + Path._used;
	CWSTR pPathBegin = Path._buf;
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

// ============================================================================ ResourceManager ===


struct ResourceManager::InternalData : public MemoryAllocatorGlobal<>
{
	std::vector<ResourceBase *> _resources;

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

	void Load(ResourceBase *res)
	{
		SIZE_T size = 0;
		BYTE *data = nullptr;
		data = Tools::LoadFile(&size, res->Path, res->GetMemoryAllocator());
		res->ResourceLoad(data, size);
	}
};

void ResourceManager::_Add(ResourceBase * res)
{
	_data->AddResource(res);
}

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

NAMESPACE_CORE_END