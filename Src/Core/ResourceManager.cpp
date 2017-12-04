#include "stdafx.h"

NAMESPACE_CORE_BEGIN

ResourceBase::ResourceBase(CSTR path, CSTR name) :
	_resourceData(nullptr),
	_resourceSize(0),
	_isLoaded(false),
	_refCounter(0)
{
	STR normalizedPath = path;
	Tools::NormalizePath(normalizedPath);

	Tools::CreateUUID(UID);
	Type = UNKNOWN;
	Path = normalizedPath;
	if (name)
	{
		Name = name;
	}
	else
	{
		CSTR pPathEnd = Path._buf + Path._used;
		CSTR pPathBegin = Path._buf;
		CSTR pEnd = pPathEnd;
		CSTR pBegin = pPathBegin;
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
}

struct ResourceManager::InternalData : public MemoryAllocatorGlobal<>
{
	std::vector<ResourceBase *> _resources;

	InternalData() {};
	~InternalData()
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

	void AddResource(CSTR path, CSTR name)
	{
		_resources.push_back(make_new<ResourceBase>(path, name));
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
	make_delete(_data);
}

ResourceBase * ResourceManager::Find(const STR & resName)
{
	for (auto &res : _data->_resources)
	{
		if (res->Name == resName)
			return res;
	}

	return nullptr;
}

ResourceBase * ResourceManager::Find(const UUID & resUID)
{
	for (auto &res : _data->_resources)
	{
		if (res->UID == resUID)
			return res;
	}

	return nullptr;
}


void ResourceManager::Add(CSTR path, CSTR name)
{
	_data->AddResource(path, name);
}

NAMESPACE_CORE_END