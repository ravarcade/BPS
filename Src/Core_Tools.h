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
	typedef void TSearchForFilesCallback(WSTR &filename, U64 size, time_t timestamp, void *ctrl);

	enum {
		directorySeparatorChar = '\\'
	};
	static constexpr const wchar_t *wDirectorySeparator = L"\\";
	static constexpr const char *directorySeparator = "\\";

	static void NormalizePath(WSTR &path);
	static void CreateUUID(UUID &uuid);
	static BYTE *LoadFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path, IMemoryAllocator *alloc);
	static bool InfoFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path);
	static bool SaveFile(SIZE_T size, const void *data, WSTR &path);
	static bool IsDirectory(WSTR &path);
	static bool Exist(WSTR &path);
	static void UUID2String(const UUID &uuid, char *buf);
	static void String2UUID(UUID &uuid, const char *buf);
	static void SearchForFiles(const WSTR &path, TSearchForFilesCallback SearchForFilesCallback, void *ctrl);
	static DWORD WinExec(WSTR &cmd, CWSTR cwd = nullptr);

	static UUID NOUID;
};
