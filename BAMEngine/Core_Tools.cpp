#include "stdafx.h"

namespace BAMS {

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

template<typename T> void _dumpArray(const char *txt, T *val, U32 cnt)
{
	TRACE("[" << txt );
	if (cnt > 1)
		TRACE("[" << cnt << "]");
	TRACE("] ");

	for (U32 i = 0; i < cnt; ++i)
	{
		if (i > 0)
			TRACE(", ");
		TRACE(val[i]);
	}
}

void Tools::Dump(BAMS::Properties * prop)
{
	for (uint32_t i = 0; i < prop->count; ++i)
	{
		auto p = prop->properties[i];
		TRACE("" << i << ": " << p.name << ": ");
		switch (p.type)
		{
		case Property::PT_EMPTY:
			TRACE("[EMPTY]");
			break;

		case Property::PT_I32:
			_dumpArray("I32", reinterpret_cast<I32*>(p.val), p.count);
			break;

		case Property::PT_U32:
			_dumpArray("U32", reinterpret_cast<U32*>(p.val), p.count);
			break;

		case Property::PT_I16:
			_dumpArray("I16", reinterpret_cast<I16*>(p.val), p.count);
			break;

		case Property::PT_U16:
			_dumpArray("U16", reinterpret_cast<U16*>(p.val), p.count);
			break;

		case Property::PT_I8:
			_dumpArray("I8", reinterpret_cast<I8*>(p.val), p.count);
			break;

		case Property::PT_U8:
			_dumpArray("U8", reinterpret_cast<U8*>(p.val), p.count);
			break;

		case Property::PT_F32:
			_dumpArray("F32", reinterpret_cast<F32*>(p.val), p.count);
			break;

		case Property::PT_CSTR:
			TRACE("[CSTR] \"" << reinterpret_cast<const char *>(p.val) << "\"");
			break;

		case Property::PT_ARRAY:
			TRACE("[ARRAY]");
			break;

		case Property::PT_TEXTURE:
			TRACE("[TEXTURE]");
			break;

		case Property::PT_UNKNOWN:
			TRACE("[UNKNOWN]");
			break;



		}
		TRACE("\n");
	}
}

// lame float to string, string to float functions
void f32toa(char *out, float v) { sprintf_s(out, 25, "%1.8e", v); }
void atof32(const char *in, float &v) { sscanf_s(in, "%f", &v); }

const char *matrixNames[] = {
	"m11", "m12", "m13", "m14",
	"m21", "m22", "m23", "m24",
	"m31", "m32", "m33", "m34",
	"m41", "m42", "m43", "m44",
};

const char xmlValueSeparator = ';';

void Tools::XMLWriteValue(tinyxml2::XMLElement * out, F32 * v, U32 count)
{
	if (count != 1)
		out->SetAttribute("count", count);

	if (count == 16)
	{ // matrix
		for (uint32_t i = 0; i < count; ++i)
			out->SetAttribute(matrixNames[i], v[i]);
	}
	else 
	{
		char *buf = new char[count * 25];
		char *p = buf;
		for (uint32_t i = 0; i < count; ++i)
		{
			if (i != 0)
				*p = xmlValueSeparator;
			++p;

			f32toa(p, v[i]);
			while (*p) ++p;			

		}
		out->SetAttribute("value", static_cast<const char *>(buf));
		delete buf;
	}
}

U32 Tools::XMLReadValue(tinyxml2::XMLElement * src, F32 *out, U32 max)
{
	U32 cnt = 0;
	cnt = src->IntAttribute("count", 1);
	max = !max ? cnt : (cnt < max ? cnt : max);

	if (out)
	{
		if (cnt == 16 && max == 16)
		{ // matrix
			for (uint32_t i = 0; i < cnt; ++i)
			{
				out[i] = src->FloatAttribute(matrixNames[i], 0.0f);
			}
		}
		else
		{
			auto buf = src->Attribute("value");
			if (buf)
			{
				auto p = buf;
				for (uint32_t i = 0; i < max; ++i)
				{
					atof32(p, out[i]);
					while (*p && *p != xmlValueSeparator) ++p;
				}
			}

		}
	}
	return cnt;
}

// ---------------------------------

template<typename T>
void SetPropertyXMLValue(tinyxml2::XMLElement * out, const Property &p)
{
	T *v = reinterpret_cast<T*>(p.val);
	STR s;

	for (auto cnt = p.count; cnt; --cnt)
	{
		s += (*v++);
		if (cnt > 1) s += ";";
	}
	out->SetAttribute("value", s.c_str());
}

template<>
void SetPropertyXMLValue<F32>(tinyxml2::XMLElement * out, const Property &p)
{
	char buf[25];
	F32 *v = reinterpret_cast<F32*>(p.val);
	STR s;

	for (auto cnt = p.count; cnt; --cnt)
	{
		f32toa(buf, *v++);
		s += buf;
		if (cnt > 1) s += ";";
	}
	out->SetAttribute("value", s.c_str());
}

template<>
void SetPropertyXMLValue<CSTR>(tinyxml2::XMLElement * out, const Property &p)
{
	if (p.val)
		out->SetAttribute("value", reinterpret_cast<CSTR>(p.val));
}

using CORE::ResBase;

template<>
void SetPropertyXMLValue<ResBase *>(tinyxml2::XMLElement * out, const Property &p)
{
	if (p.val)
		out->SetAttribute("value", reinterpret_cast<ResBase *>(p.val)->Name.c_str());
}

void SetPropertyXMLValue(tinyxml2::XMLElement * out, const Property &p, Tools::TNewXMLElementFunc NewXMLElement)
{
	CSTR *v = reinterpret_cast<CSTR*>(p.val);
	for (auto cnt = p.count; cnt; --cnt)
	{
		tinyxml2::XMLElement *entry = NewXMLElement("str");
		entry->SetText(*v++);
		out->InsertEndChild(entry);
	}
}

void XMLWriteVal(tinyxml2::XMLElement * parent, const Property &p, Tools::TNewXMLElementFunc NewXMLElement)
{
	tinyxml2::XMLElement *out = NewXMLElement(p.name);

	//out->SetAttribute("name", p.name);
	out->SetAttribute("type", p.type);	
	switch (p.type)
	{
	case Property::PT_I8: SetPropertyXMLValue<I8>(out, p); break;
	case Property::PT_U8: SetPropertyXMLValue<U8>(out, p); break;
	case Property::PT_I16: SetPropertyXMLValue<I16>(out, p); break;
	case Property::PT_U16: SetPropertyXMLValue<U16>(out, p); break;
	case Property::PT_I32: SetPropertyXMLValue<I32>(out, p); break;
	case Property::PT_U32: SetPropertyXMLValue<U32>(out, p); break;
	case Property::PT_F32: SetPropertyXMLValue<F32>(out, p); break;
	case Property::PT_CSTR: SetPropertyXMLValue<CSTR>(out, p); break;
//	case Property::PT_ARRAY: return false; // not supported
	case Property::PT_TEXTURE: SetPropertyXMLValue<ResBase *>(out, p); break;
	default:
		return;
	}
	parent->InsertEndChild(out);
}

void Tools::XMLWriteProperties(tinyxml2::XMLElement * out, const Properties &prop, TNewXMLElementFunc NewXMLElement)
{
	if (!NewXMLElement)
		NewXMLElement = [](CSTR s) {return BAMS::CORE::globalResourceManager->NewXMLElement(s); };

	for (uint32_t i = 0; i < prop.count; ++i)
	{
		XMLWriteVal(out, prop.properties[i], NewXMLElement);
	}
}

// --------------------------------------------------------------------------------

void GoToNextValue(const char *&s)
{
	while (*s && *s != xmlValueSeparator)
		++s;
	if (*s == xmlValueSeparator)
		++s;
}

template<typename T>
void GetXMLValue(Property *p, const char *s)
{
	auto o = reinterpret_cast<T *>(p->val);
	do {
		*o++ = static_cast<T>(atol(s));
		GoToNextValue(s);
	} while (*s);
}


template<>
void GetXMLValue<F32>(Property *p, const char *s)
{
	auto o = reinterpret_cast<F32 *>(p->val);
	do {
		atof32(s, *o++);
		GoToNextValue(s);
	} while (*s);
}

U32 xmlEntryGetCount(tinyxml2::XMLElement *xmlEntry)
{
	U32 ret = 0;
	int type = xmlEntry->IntAttribute("type", 0);
	if (type == Property::PT_CSTR)
	{
		for (auto x = xmlEntry->FirstChildElement(); x; x = x->NextSiblingElement())
			++ret;

	}
	else {
		++ret;
		for (auto v = xmlEntry->Attribute("value"); *v; ++v)
			if (*v == xmlValueSeparator)
				++ret;
	}
	return ret;
}

void Tools::XMLReadProperties(tinyxml2::XMLElement * src, sProperties &prop)
{
	if (!src)
		return;
	prop.clear();
	for (auto xmlEntry = src->FirstChildElement(); xmlEntry; xmlEntry = xmlEntry->NextSiblingElement())
	{
		auto name = xmlEntry->Name();
		auto type = xmlEntry->IntAttribute("type", 0);
		auto count = xmlEntryGetCount(xmlEntry);
		auto p = prop.add(name, type, count);
		
		switch (type)
		{
		case Property::PT_I8: GetXMLValue<I8>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_U8: GetXMLValue<U8>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_I16: GetXMLValue<I16>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_U16: GetXMLValue<U16>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_I32: GetXMLValue<I32>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_U32: GetXMLValue<U32>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_F32: GetXMLValue<F32>(p, xmlEntry->Attribute("value")); break;
		case Property::PT_CSTR: p->val = const_cast<char *>(prop.storeString(xmlEntry->Attribute("value")));  break;
		case Property::PT_TEXTURE: p->val = CORE::globalResourceManager->FindExisting(xmlEntry->Attribute("value"), RESID_IMAGE); break;
		}	
	}
}

time_t Tools::TimestampNow()
{

	return std::time(nullptr);
}

void Tools::Mat4mul(F32 * O, const F32 * A, const F32 * B)
{
	O[0] = A[0] * B[0] + A[1] * B[4] + A[2] * B[8] + A[3] * B[12];
	O[1] = A[0] * B[1] + A[1] * B[5] + A[2] * B[9] + A[3] * B[13];
	O[2] = A[0] * B[2] + A[1] * B[6] + A[2] * B[10] + A[3] * B[14];
	O[3] = A[0] * B[3] + A[1] * B[7] + A[2] * B[11] + A[3] * B[15];

	O[4] = A[4] * B[0] + A[5] * B[4] + A[6] * B[8] + A[7] * B[12];
	O[5] = A[4] * B[1] + A[5] * B[5] + A[6] * B[9] + A[7] * B[13];
	O[6] = A[4] * B[2] + A[5] * B[6] + A[6] * B[10] + A[7] * B[14];
	O[7] = A[4] * B[3] + A[5] * B[7] + A[6] * B[11] + A[7] * B[15];

	O[8] = A[8] * B[0] + A[9] * B[4] + A[10] * B[8] + A[11] * B[12];
	O[9] = A[8] * B[1] + A[9] * B[5] + A[10] * B[9] + A[11] * B[13];
	O[10] = A[8] * B[2] + A[9] * B[6] + A[10] * B[10] + A[11] * B[14];
	O[11] = A[8] * B[3] + A[9] * B[7] + A[10] * B[11] + A[11] * B[15];

	O[12] = A[12] * B[0] + A[13] * B[4] + A[14] * B[8] + A[15] * B[12];
	O[13] = A[12] * B[1] + A[13] * B[5] + A[14] * B[9] + A[15] * B[13];
	O[14] = A[12] * B[2] + A[13] * B[6] + A[14] * B[10] + A[15] * B[14];
	O[15] = A[12] * B[3] + A[13] * B[7] + A[14] * B[11] + A[15] * B[15];
}

// ============================================================================================= mouse
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

LPDIRECTINPUT8 pDI;
LPDIRECTINPUTDEVICE8 pMouse = nullptr;
bool isInitialized = false;
bool bImmediate = false;


bool InitMouse()
{
	isInitialized = false;
	DWORD hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
		IID_IDirectInput8, (VOID**)&pDI, NULL);
	if (FAILED(hr)) return false;

	hr = pDI->CreateDevice(GUID_SysMouse, &pMouse, NULL);
	if (FAILED(hr)) return false;

	hr = pMouse->SetDataFormat(&c_dfDIMouse2);
	if (FAILED(hr)) return false;

	if (!bImmediate)
	{
		DIPROPDWORD dipdw;
		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
		dipdw.diph.dwObj = 0;
		dipdw.diph.dwHow = DIPH_DEVICE;
		dipdw.dwData = 16; // Arbitrary buffer size

		if (FAILED(hr = pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
			return false;
	}

	pMouse->Acquire();
	isInitialized = true;
	return true;
}

bool Tools::ReadMouse(int *xPosRelative, int *yPosRelative, uint32_t *buttons, int *zPosRelative)
{
	DIMOUSESTATE2 dims2;
	ZeroMemory(&dims2, sizeof(dims2));

	if (!pMouse)
		InitMouse();

	DWORD hr = pMouse->GetDeviceState(sizeof(DIMOUSESTATE2), &dims2);

	if (FAILED(hr))
	{
		hr = pMouse->Acquire();
		while (hr == DIERR_INPUTLOST)
			hr = pMouse->Acquire();

		// no mouse data
		return false;
	}

	*xPosRelative = dims2.lX;
	*yPosRelative = dims2.lY;
	if (zPosRelative)
		*zPosRelative = dims2.lZ;
	if (buttons)
	{
		uint32_t bit = 1;
		uint32_t result = 0;
		for (uint32_t i = 0; i < 5; ++i)
		{
			if (dims2.rgbButtons[i] & 0x80)
			{
				result |= bit;
			}
			bit += bit;
		}
		*buttons = result;
	}
	

	return true;
}

UUID Tools::NOUID = { 0 };

}; // BAMS namespace
