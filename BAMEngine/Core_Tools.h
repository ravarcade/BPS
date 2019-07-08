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
	typedef tinyxml2::XMLElement * (*TNewXMLElementFunc)(const char *name);

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

	static void Dump(BAMS::Properties *prop);

	static void XMLWriteValue(tinyxml2::XMLElement * out, F32 *v, U32 count);
	static U32 XMLReadValue(tinyxml2::XMLElement * src, F32 *out, U32 max = 0);
	static void XMLWriteProperties(tinyxml2::XMLElement * out, const Properties &prop, TNewXMLElementFunc NewXMLElement = nullptr);
	static void XMLReadProperties(tinyxml2::XMLElement * src, sProperties &prop);

	static time_t TimestampNow();
	static void Mat4mul(F32 *O, const F32 *A, const F32 *B);

	static bool ReadMouse(int *xPosRelative, int *yPosRelative, uint32_t *buttons = nullptr, int *zPosRelative = nullptr);
};
