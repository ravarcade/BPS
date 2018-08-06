/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/


class Tools
{
public:
	enum {
		directorySeparatorChar = '\\'
	};
	static constexpr const wchar_t *wDirectorySeparator = L"\\";
	static constexpr const char *directorySeparator = "\\";

	static void NormalizePath(WSTR &path);
	static void CreateUUID(UUID &uuid);
	static BYTE *LoadFile(SIZE_T *pFileSize, WSTR &path, IMemoryAllocator *alloc);

	static UUID NOUID;
};
