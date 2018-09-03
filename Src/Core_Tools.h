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
	static BYTE *LoadFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path, IMemoryAllocator *alloc);
	static void InfoFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path);
	static bool SaveFile(SIZE_T size, const void *data, WSTR &path);
	static void UUID2String(UUID &uuid, char *buf);

	static UUID NOUID;
};
