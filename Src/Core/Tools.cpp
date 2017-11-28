#include "stdafx.h"

NAMESPACE_CORE_BEGIN

void Tools::NormalizePath(STR & path)
{
	// replace all "/" with "\"
	for (unsigned int i=0; i<path.size(); ++i)
	{
		auto &x = path[i];
		if (x == '/' || x == '\\')
		{
			x = Tools::directorySeparatorChar;
		}
	}
}

void Tools::CreateUUID(UUID &uuid)
{
	memset(&uuid, 0, sizeof(UUID));
	UuidCreate(&uuid);
}

UUID Tools::NOUID = { 0 };

NAMESPACE_CORE_END