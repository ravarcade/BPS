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
	static void NormalizePath(WSTR &path);
	static void CreateUUID(UUID &uuid);
	static BYTE *LoadFile(SIZE_T *pFileSize, WSTR &path, IMemoryAllocator *alloc);

	static UUID NOUID;
};
