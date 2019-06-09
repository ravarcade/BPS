#include "stdafx.h"

namespace BAMS {
namespace CORE {

// ============================================================================ ResBase ===

void ResBase::_CreateResourceImplementation()
{
	// create resource
	globalResourceManager->CreateResourceImplementation(this);
}

/*
void ResBase::Init(CWSTR path)
{
	_resData = nullptr;
	_resSize = 0;
	_isLoaded = false;
	_refCounter = 0;
	Tools::CreateUUID(UID);

	WSTR normalizedPath = path;
	Tools::NormalizePath(normalizedPath);

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

void ResBase::Init(CSTR name)
{
	_resData = nullptr;
	_resSize = 0;
	_isLoaded = false;
	_refCounter = 0;
	// we do nothing with Path, by default it is empty
	Name = name;
	Tools::CreateUUID(UID);
}
*/
}; // CORE namespace
}; // BAMS namespace