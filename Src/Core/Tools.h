/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
*
*/


class Tools
{
public:
	enum {
		directorySeparatorChar = '\\'
	};
	static void NormalizePath(STR & path);
	static void CreateUUID(UUID &uuid);
	static UUID NOUID;
};
