#include "stdafx.h"
#include "..\..\BAMEngine\\stdafx.h"
#include <stdio.h>

NAMESPACE_CORE_BEGIN

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
}

void Tools::CreateUUID(UUID &uuid)
{
	memset(&uuid, 0, sizeof(UUID));
	UuidCreate(&uuid);
}

BYTE * Tools::LoadFile(SIZE_T *pFileSize, WSTR &path, IMemoryAllocator * alloc)
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

UUID Tools::NOUID = { 0 };

NAMESPACE_CORE_END