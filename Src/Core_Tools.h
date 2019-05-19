/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/


class BAMS_EXPORT Tools
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
	static bool SaveFile(WSTR &path, const void *data, SIZE_T size);
	static bool IsDirectory(WSTR &path);
	static bool Exist(WSTR &path);
	static void UUID2String(const UUID &uuid, char *buf);
	static void String2UUID(UUID &uuid, const char *buf);
	static void SearchForFiles(const WSTR &path, TSearchForFilesCallback SearchForFilesCallback, void *ctrl);
	static DWORD WinExec(WSTR &cmd, CWSTR cwd = nullptr);
	static int FindMatchingFileExtension(const wchar_t * const fn, const wchar_t * const ext, bool caseInsesitive = true);
	static int FindMatchingFileExtension(const char * const fn, const char * const ext, bool caseInsesitive = true);

	static UUID NOUID;
};
