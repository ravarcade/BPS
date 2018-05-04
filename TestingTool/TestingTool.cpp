// TestingTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BAMEngine.h"


void DumpHex(BYTE *data, size_t s)
{
	const char *hex = "0123456789abcdef";
	for (size_t i = 0; i < s; ++i)
	{
		static char txt[3] = { 0, 0, 0 };
		int c = data[i];
		txt[0] = hex[(c >> 4) & 0xf];
		txt[1] = hex[c & 0xf];
		printf(txt);
		if ((i % 4) == 3)
			printf(" ");		
	}
	printf("|");
	for (size_t i = 0; i < s; ++i)
	{
		static char txt[2] = { 0, 0};
		int c = data[i];
		txt[0] = isprint(c) ? c : '.';
		printf(txt);
	}
}

void DumpRAM()
{
	size_t size, counter;
	void *current = nullptr;
	void *data = nullptr;
	
	while (BAMS::GetMemoryBlocks(&current, &size, &counter, &data))
	{
		if (current == nullptr)
			break;

		printf("[%5d] : %d bytes : ", (int)counter, (int)size);
		size_t s = size < 16 ? size : 16;
		DumpHex((BYTE *)data, s);
		printf("\n");
	}

	uint64_t Max, Current, Counter;
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	printf("\n=========================================================\nMax = %d, Current = %d, Count = %d\n", (int)Max, (int)Current, (int)Counter);
}

#include <vector>
using std::vector;

SHORT AllKeyboardStates[256] = { 0 };
DWORD AllKeyboardRepeatTime[256];
bool currentKeyState[256];
#pragma comment(lib, "Winmm.lib")

void updateAllKeysScan()
{
	const DWORD rep_first = 300;
	const DWORD rep_next = 50;

	DWORD ct = timeGetTime();
	for (int i = 0; i < 256; ++i)
	{
		auto r = GetAsyncKeyState(i);
		auto o = AllKeyboardStates[i];
		if (i <= 0x20) {
			r = r & 0x8000 ? 0x8001 : 0;
		}

		r = r << 8;
		if (r != 0 && o == 0)
		{
			currentKeyState[i] = true;
			AllKeyboardRepeatTime[i] = ct + rep_first;
		}
		else if (r != 0 && AllKeyboardRepeatTime[i] < ct)
		{
			currentKeyState[i] = true;
			AllKeyboardRepeatTime[i] = ct + rep_next;
		}
		else {
			currentKeyState[i] = false;
		}
		AllKeyboardStates[i] = r;
	}
}

bool GetKeyPressed(DWORD key)
{
	return currentKeyState[key];
}

void wait_for_esc()
{
	for (;;)
	{
//		Sleep(100);
		SleepEx(100, TRUE);
		updateAllKeysScan();
		if (GetKeyPressed(VK_ESCAPE))
			break;

	}
}

int main()
{
	{
		vector<int> x(3);
	}

	uint64_t Max, Current, Counter;

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	BAMS::Initialize();

	{
		BAMS::DoTests();
		BAMS::CResourceManager rm;
		rm.StartDirectoryMonitor();
		rm.AddDir(L"C:\\Work\\test");
//		rm.AddDir(_tgetenv(_T("USERPROFILE")));
//		rm.AddDir(L"C:\\");

		rm.AddResource(L"C:\\Work\\BPS\\BAMEngine\\ReadMe.txt");
		auto r = rm.GetRawDataByName("ReadMe");
		rm.LoadSync();
		auto ruid = r.GetUID();
		auto rpath = r.GetPath();
		auto rname = r.GetName();
		auto rdata = r.GetData();
		auto rsize = r.GetSize();

		auto res = rm.FindByName("ReadMe");
		auto uid = res.GetUID();
		auto path = res.GetPath();
		auto name = res.GetName();

		auto rr = rm.GetRawDataByUID(uid);
		uid = rr.GetUID();
		path = rr.GetPath();
		name = rr.GetName();


		BAMS::IResource *resList[10];
		uint32_t resCount = 10;
		rm.Filter(resList, &resCount, "*");
		wait_for_esc();
	}

	BAMS::Finalize();
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	wait_for_esc();
	return 0;
}

