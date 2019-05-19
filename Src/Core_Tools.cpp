#include "stdafx.h"

namespace BAMS {
//namespace CORE {

#define TICKS_PER_SECOND 10000000
#define EPOCH_DIFFERENCE 11644473600LL

void Tools::NormalizePath(WSTR & path)
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

	// remove ending directory separator char (if exist)
	if (path[path.size() - 1] == Tools::directorySeparatorChar)
		path.resize((U32)path.size() - 1);
}

void Tools::CreateUUID(UUID &uuid)
{
	memset(&uuid, 0, sizeof(UUID));
	UuidCreate(&uuid);
}

void Tools::UUID2String(const UUID &g, char *buf)
{
	sprintf_s(buf, 39, "[%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]", g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
		g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
}

inline U8 xdigit(int c)
{
	return c >= '0' && c <= '9' ? U8(c - '0') :
		c >= 'a' ? U8(c - ('a' - 10)) : U8(c - ('A' - 10));
}

void hexstring2value(BYTE *out, const char *&src, int len, bool bigEndian = false)
{
	int base = len-1;
	while (len && *src)
	{
		if (isxdigit(*src))
		{
			U8 b = xdigit(src[0]) << 4 | xdigit(src[1]);
			--len;
			out[bigEndian ? base - len : len] = b;
			++src;
		}
		++src;
	}
}

void Tools::String2UUID(UUID &uuid, const char *buf)
{	
	hexstring2value((BYTE *)&uuid.Data1, buf, sizeof(uuid.Data1));
	hexstring2value((BYTE *)&uuid.Data2, buf, sizeof(uuid.Data2));
	hexstring2value((BYTE *)&uuid.Data3, buf, sizeof(uuid.Data3));
	hexstring2value((BYTE *)&uuid.Data4, buf, sizeof(uuid.Data4), true); // it is set of 8 bytes, we can read it at once if we read it as 128 bit big endian value
}

BYTE * Tools::LoadFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path, IMemoryAllocator * alloc)
{
	*pFileSize = 0;
	HANDLE hFile;
	OVERLAPPED ol = { 0 };
	
	hFile = CreateFile(path.c_str(), GENERIC_READ,
		FILE_SHARE_READ, //FILE_SHARE_READ | FILE_FLAG_OVERLAPPED, 
		NULL, 
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL, 
		NULL);

	DWORD err = GetLastError();

	if (hFile == INVALID_HANDLE_VALUE)
		return nullptr;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
	{
		DWORD err = GetLastError();
		CloseHandle(hFile);
		return nullptr; // error condition, could call GetLastError to find out more
	}

	if (pTimestamp)
	{
		FILETIME lastWriteTime;
		GetFileTime(hFile, nullptr, nullptr, &lastWriteTime);
		U64 ft = U64(lastWriteTime.dwHighDateTime) << 32 | lastWriteTime.dwLowDateTime;
		*pTimestamp = ft / TICKS_PER_SECOND - EPOCH_DIFFERENCE;
	}

	BYTE *binBuf = static_cast<BYTE *>(alloc->allocate(size.QuadPart));
	if (!binBuf)
	{
		CloseHandle(hFile);
		return nullptr;
	}
	
	SIZE_T bytesToRead = size.QuadPart;
	static DWORD step = 1024 * 1024 * 64;
	BYTE *buf = binBuf;

	while (bytesToRead)
	{
		DWORD blockSize = bytesToRead > step ? step : static_cast<DWORD>(bytesToRead);
		DWORD bytesReaded = 0;
		if (FALSE == ReadFile(hFile, buf, blockSize, &bytesReaded, &ol))
		{
			DWORD err = GetLastError();
			CloseHandle(hFile);
			alloc->deallocate(binBuf);
			return nullptr;
		}
		buf += bytesReaded;
		bytesToRead -= bytesReaded;
	}

	// happy end.
	CloseHandle(hFile);

	*pFileSize = size.QuadPart;
	return binBuf;
}
/*
time_t Tools::GetTimestamp(const WSTR & fname)
{
	HANDLE hFile;
	FILETIME lastWriteTime;
	OVERLAPPED ol = { 0 };

	hFile = CreateFile(fname.c_str(), GENERIC_READ,
		FILE_SHARE_READ, //FILE_SHARE_READ | FILE_FLAG_OVERLAPPED, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	if (hFile == INVALID_HANDLE_VALUE)
		return 0;

	GetFileTime(hFile, nullptr, nullptr, &lastWriteTime);
	U64 ft = U64(lastWriteTime.dwHighDateTime) << 32 | lastWriteTime.dwLowDateTime;
	time_t t = ft / TICKS_PER_SECOND - EPOCH_DIFFERENCE;

	CloseHandle(hFile);
	return t;
}
*/
bool Tools::InfoFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path)
{
	if (!path.size())
		return true;
	HANDLE hFile;
	OVERLAPPED ol = { 0 };

	hFile = CreateFile(path.c_str(), GENERIC_READ,
		FILE_SHARE_READ, //FILE_SHARE_READ | FILE_FLAG_OVERLAPPED, 
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD err = GetLastError();

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
	{
		DWORD err = GetLastError();
		CloseHandle(hFile);
		return false; // error condition, could call GetLastError to find out more
	}

	if (pTimestamp)
	{
		FILETIME lastWriteTime;
		GetFileTime(hFile, nullptr, nullptr, &lastWriteTime);
		U64 ft = U64(lastWriteTime.dwHighDateTime) << 32 | lastWriteTime.dwLowDateTime;
		*pTimestamp = ft / TICKS_PER_SECOND - EPOCH_DIFFERENCE;
	}

	CloseHandle(hFile);
	
	if (pFileSize)
		*pFileSize = size.QuadPart;

	return true;
}

bool Tools::SaveFile(WSTR &path, const void *data, SIZE_T size)
{
	HANDLE hFile;
	OVERLAPPED ol = { 0 };

	hFile = CreateFile(path.c_str(), GENERIC_WRITE,
		0,
		NULL,
		CREATE_ALWAYS,
		FILE_ATTRIBUTE_NORMAL,
		NULL);

	DWORD err = GetLastError();

	if (hFile == INVALID_HANDLE_VALUE)
		return false;

	if (WriteFile(hFile, data, (DWORD)size, NULL, NULL) == FALSE)
	{
		err = GetLastError();
		return false;
	}

	CloseHandle(hFile);
	return true;
}

bool Tools::IsDirectory(WSTR &path)
{
	DWORD attr = GetFileAttributesW(path.c_str());
	return (attr & FILE_ATTRIBUTE_DIRECTORY) != 0;
}

bool Tools::Exist(WSTR &path)
{
	if (!path.size())
		return true;

	DWORD dwAttrib = GetFileAttributes(path.c_str());

	return (dwAttrib != INVALID_FILE_ATTRIBUTES &&
		!(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

void Tools::SearchForFiles(const WSTR &path, TSearchForFilesCallback SearchForFilesCallback, void *ctrl)
{
	HANDLE hFind;
	WIN32_FIND_DATA data;
	WSTR fpath = path;
	if (fpath[fpath.size() - 1] != directorySeparatorChar)
		fpath += wDirectorySeparator;
	WSTR mask = fpath + L"*.*";

	hFind = FindFirstFileW(mask.c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return;

	WSTR filename;
	do {

		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
		{
			if (wcscmp(data.cFileName, L".") != 0 && wcscmp(data.cFileName, L"..") != 0)
			{
				// check subdir
				filename = fpath + data.cFileName;
				SearchForFiles(filename, SearchForFilesCallback, ctrl);
			}
		}
		else {
			filename = fpath + data.cFileName;
			U64 size = U64(data.nFileSizeHigh) << 32 | data.nFileSizeLow;
			U64 ft   = U64(data.ftLastWriteTime.dwHighDateTime) << 32 | data.ftLastWriteTime.dwLowDateTime;
			time_t timestamp = ft / TICKS_PER_SECOND - EPOCH_DIFFERENCE;
			SearchForFilesCallback(filename, size, timestamp, ctrl);
		}
	} while (FindNextFile(hFind, &data));

	FindClose(hFind);
}

/// <summary>
/// Execute command in windows shell.
/// see: https://docs.microsoft.com/en-us/windows/desktop/api/winbase/nf-winbase-winexec
/// </summary>
/// <param name="cmd">The command.</param>
DWORD Tools::WinExec(WSTR &cmd, CWSTR cwd)
{
	DWORD exitCode = 0;
	SECURITY_ATTRIBUTES sa = { 0 };
	sa.nLength = sizeof(sa);
	sa.lpSecurityDescriptor = NULL;
	sa.bInheritHandle = TRUE;

	HANDLE hStdOutRd, hStdOutWr;
	HANDLE hStdErrRd, hStdErrWr;

	if (!CreatePipe(&hStdOutRd, &hStdOutWr, &sa, 0))
	{
		// error handling...
		TRACE("Tools::CmdExec: fail to create StdOut pipes\n");
		return -1;
	}

	if (!CreatePipe(&hStdErrRd, &hStdErrWr, &sa, 0))
	{
		// error handling...
		TRACE("Tools::CmdExec: fail to create StdErr pipes\n");
		CloseHandle(hStdOutRd);
		CloseHandle(hStdOutWr);
		return -1;
	}

	SetHandleInformation(hStdOutRd, HANDLE_FLAG_INHERIT, 0);
	SetHandleInformation(hStdErrRd, HANDLE_FLAG_INHERIT, 0);

	STARTUPINFO si = { 0 };
	si.cb = sizeof(si);
	si.dwFlags = STARTF_USESTDHANDLES;
	si.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
	si.hStdOutput = hStdOutWr;
	si.hStdError = hStdErrWr;

	PROCESS_INFORMATION pi = { 0 };

	if (!CreateProcessW(NULL, const_cast<wchar_t *>(cmd.c_str()), NULL, NULL, TRUE, 0, NULL, cwd, &si, &pi))
	{
		// error handling...
		TRACE("Tools::CmdExec: fail to create process\n");
	}
	else
	{
		// read from hStdOutRd and hStdErrRd as needed until the process is terminated...
		COMMTIMEOUTS timeouts = { 0, //interval timeout. 0 = not used
						  0, // read multiplier
						100, // read constant (milliseconds)
						  0, // Write multiplier
						  0  // Write Constant
		};


		SetCommTimeouts(hStdOutRd, &timeouts);
		const size_t BUFSIZE = 8000;
		char chBuf[BUFSIZE+1];
		DWORD dwRead;


		while (GetExitCodeProcess(pi.hProcess, &exitCode) && exitCode == STILL_ACTIVE)
		{
			ReadFile(hStdOutRd, chBuf, BUFSIZE, &dwRead, 0);
			if (dwRead == 0) {
				//insert code to handle timeout here
				TRACE(".");
			}
			else {
				chBuf[dwRead] = 0;
				TRACE(chBuf);
			}
		}

		CloseHandle(pi.hThread);
		CloseHandle(pi.hProcess);
	}

	

	CloseHandle(hStdOutRd);
	CloseHandle(hStdOutWr);
	CloseHandle(hStdErrRd);
	CloseHandle(hStdErrWr);

	return exitCode;
}

size_t slen(const wchar_t *s) { return wcslen(s); }
size_t slen(const char *s) { return strlen(s); }
size_t sspn(const wchar_t *s, const wchar_t *pat) { return wcsspn(s, pat); }
size_t sspn(const char *s, const char *pat) { return strspn(s, pat); }
const wchar_t *spbrk(const wchar_t *s, const wchar_t *pat) { return wcspbrk(s, pat); }
const char *spbrk(const char *s, const char *pat) { return strpbrk(s, pat); }

template<typename T>
bool endsWith(T s, size_t slen, T p, size_t plen, bool caseInsesitive = true)
{
	if (plen > slen)
		return false;

	s += slen - plen;
	while (*s)
	{
		if (caseInsesitive ? towlower(*s) != towlower(*p) : *s != *p)
			return false;
		++s;
		++p;
	}
	return true;
}

template<typename T>
bool endsWith(T s, size_t sl, T p, bool caseInsesitive = true) { return endsWith(s, sl, p, slen(p), caseInsesitive); }

template<typename T>
const T *strtoken(const T *s, const T *delim, size_t &len)
{
	static const T *olds;
	const T *token;

	if (!s) 
		s = olds;

	s += sspn(s, delim);
	if (!*s)
	{
		olds = s;
		len = 0;
		return nullptr;
	}

	token = s;
	s = spbrk(token, delim);
	if (s == nullptr)
	{
		len = slen(token);
		olds = token + len;
	}
	else {
		len = s - token;
		olds = s + 1;
	}

	return token;
}

/// <summary>
/// Finds the matching file extension.
/// Examples:
/// FindMatchingFileExtension(L"hello.bmp", "pcx;bmp;jpeg") = 1
/// FindMatchingFileExtension(L"hello.zip", "pcx;bmp;jpeg") = -1
/// </summary>
/// <param name="fn">The file name.</param>
/// <param name="ext">The extensions.</param>
/// <param name="caseInsesitive">if set to <c>true</c> [case insesitive].</param>
/// <returns></returns>
int Tools::FindMatchingFileExtension(const wchar_t * const fn, const wchar_t * const ext, bool caseInsesitive)
{
	size_t len = slen(fn);
	size_t szlen = 0;

	int ret = 0;
	const wchar_t* sz = strtoken(ext, L";", szlen);
	do {
		if (*sz == '*') { ++sz; --szlen; }
		if (*sz == '.') { ++sz; --szlen; }
		if (endsWith(fn, len, sz, szlen, caseInsesitive))
		{
			if (szlen < len && fn[len-szlen-1] == '.')
				return ret;
		}
		++ret;
	} while ((sz = strtoken<wchar_t>(nullptr, L";", szlen)));
	return -1;
}

int Tools::FindMatchingFileExtension(const char * const fn, const char * const ext, bool caseInsesitive)
{
	size_t len = slen(fn);
	size_t szlen = 0;

	int ret = 0;
	const char* sz = strtoken(ext, ";", szlen);
	do {
		if (*sz == '*') { ++sz; --szlen; }
		if (*sz == '.') { ++sz; --szlen; }
		if (endsWith(fn, len, sz))
		{
			return ret;
		}
		++ret;
	} while ((sz = strtoken<char>(nullptr, ";", szlen)));
	return -1;
}

UUID Tools::NOUID = { 0 };

//}; // CORE namespace
}; // BAMS namespace
