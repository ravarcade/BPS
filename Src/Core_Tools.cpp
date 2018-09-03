#include "stdafx.h"
#include <stdio.h>

NAMESPACE_CORE_BEGIN

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
	if (path[path.size() - 1] == Tools::directorySeparatorChar)
		path.resize((U32)path.size() - 1);
}

void Tools::CreateUUID(UUID &uuid)
{
	memset(&uuid, 0, sizeof(UUID));
	UuidCreate(&uuid);
}

void Tools::UUID2String(UUID &g, char *buf)
{
	sprintf_s(buf, 39, "[%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]", g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
		g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
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

void Tools::InfoFile(SIZE_T *pFileSize, time_t *pTimestamp, WSTR &path)
{
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
		return;

	LARGE_INTEGER size;
	if (!GetFileSizeEx(hFile, &size))
	{
		DWORD err = GetLastError();
		CloseHandle(hFile);
		return; // error condition, could call GetLastError to find out more
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
}

bool Tools::SaveFile(SIZE_T size, const void *data, WSTR &path)
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

UUID Tools::NOUID = { 0 };

NAMESPACE_CORE_END