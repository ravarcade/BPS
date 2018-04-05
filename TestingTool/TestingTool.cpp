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

int main()
{

	uint64_t Max, Current, Counter;

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	{
		BAMS::Initialize();
		BAMS::DoTests();
		{
			BAMS::CResourceManager rm;



			//	S1 st1;
			//	S1 st1;
				/*
				S2 st2("hello world");
				S3 st3(st2);
				foo(std::move(st3));
			//	S2 sst1;
			//	sst1 = st2;

				st1 += " and you";
				st2 += S3("!tada!");
				st1 = (st2 + "haha" + st1);
				st3 += st3;
				st3 = st3 + st3 + st3;
				*/

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
		}
		BAMS::Finalize();
	}
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	return 0;
}

