// TestingTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BAMEngine.h"
using namespace BAMS;

void LoadVK()
{
	auto hm = LoadLibrary(L"RenderingEngineVK.dll");

	typedef void func(void);

	func *RegisterModule = (func *)GetProcAddress(hm, "RenderingEngine_RegisterModule");
	if (RegisterModule)
		RegisterModule();	
}

const char * UUID2String(GUID &g)
{
	static char tmp[256];
	//[Guid("5FE64E68-7A75-4D58-B0B0-89A29C5D80E1")]
	sprintf_s(tmp, "[%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x]", g.Data1, g.Data2, g.Data3,
		g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
		g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);

	return tmp;
};


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
	printf("| ");
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

BYTE AllKeyboardStates[256] = { 0 };
DWORD AllKeyboardRepeatTime[256];
BYTE currentKeyState[256];
#pragma comment(lib, "Winmm.lib")

void updateAllKeysScan()
{
	const DWORD rep_first = 300;
	const DWORD rep_next = 50;
	BYTE ks[256];
	DWORD ct = timeGetTime();
	GetKeyboardState(ks);

	for (int i = 0; i < 256; ++i)
	{
//		auto r = ks[i];
		auto r = GetAsyncKeyState(i) >> 8;
		auto o = AllKeyboardStates[i];
		r = r & 0x80 ? 0x80 : 0;
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
		BAMS::CEngine::Update(25.0f);
		SleepEx(25, TRUE);
		updateAllKeysScan();
		if (GetKeyPressed(VK_ESCAPE))
			break;

	}
}

static PCREATE_WINDOW w0 = { 0, 1200, 800, 10, 10 };
static PCREATE_WINDOW w1 = { 1, 500, 500, 1310, 10 };
static PCREATE_WINDOW w2 = { 2, 500, 200, 1310, 610 };

void testloop(BAMS::CEngine &en)
{
	static PADD_MODEL c0 = { 0, "cubename", "cube", "cube" };
	static PADD_MODEL c02 = { 0, "cubename", "cube", "default" };
	static PADD_MODEL c1 = { 1, "cubename", "cube", "default" };
	static PADD_MODEL c2 = { 2, "cubename", "cube", "default" };
	static PCLOSE_WINDOW cw0 = { 0 };
	static PCLOSE_WINDOW cw1 = { 1 };
	static PCLOSE_WINDOW cw2 = { 2 };

	static bool w0v = true;
	static bool w1v = false;
	static bool w2v = false;

	for (bool isRunning = true; isRunning;)
	{
		//		Sleep(100);
		BAMS::CEngine::Update(25.0f);
		SleepEx(25, TRUE);
		updateAllKeysScan();
		for (uint16_t i = 0; i < sizeof(currentKeyState); ++i)
		{
			if (currentKeyState[i])
			{
				switch (i) {
				case VK_ESCAPE:
					isRunning = false;
						break;
				case VK_ADD:
					en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, nullptr);
					break;

				case '1':
					if (w0v)
						en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw0);
					else
						en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, &w0);
					w0v = !w0v;
					break;
				case '2':
					if (w1v)
						en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw1);
					else
						en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, &w1);
					w1v = !w1v;
					break;
				case '3':
					if (w2v)
						en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw2);
					else
						en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, &w2);
					w2v = !w2v;
					break;

				case 'Q':
					en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &c0);
					break;

				case 'A':
					en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &c02);
					break;

				case 'W':
					en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &c1);
					break;
				case 'E':
					en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &c2);
					break;

				}
			}
		}
	}
}

int main()
{

	uint64_t Max, Current, Counter;

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	
	BAMS::Initialize(); // Starts Game Engine Thread, it will create all needed threads ...
	LoadVK();
	{
		BAMS::CEngine en;

		BAMS::RENDERINENGINE::VertexDescription vd;


		BAMS::DoTests();
			BAMS::CResourceManager rm;
			rm.RootDir(L"C:\\Work\\test");
			rm.LoadSync();
			en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, &w0);

			rm.AddResource(L"C:\\Work\\BPS\\BAMEngine\\ReadMe.txt");
			
			// default shader program ... not needed any more
//			auto s = rm.GetShaderByName("default");
//			s.AddProgram(L"/Shaders/default.vert.glsl");
//			s.AddProgram(L"/Shaders/default.frag.glsl");

			auto r = rm.GetRawDataByName("ReadMe");
			printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
			rm.LoadSync();
			printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
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


			// loop
			testloop(en);

			rm.LoadSync();

			BAMS::IResource *resList[100];
			uint32_t resCount = 100;
			rm.Filter(resList, &resCount, "*");
			for (uint32_t i = 0; i < resCount; ++i)
			{
				BAMS::CResource r(resList[i]);
				if (!r.IsLoaded())
					rm.LoadSync();
				printf("%3d: \"%s\", %#010x, %s", i, r.GetName(), r.GetType(), UUID2String(r.GetUID()));
				wprintf(L", \"%s\"\n", r.GetPath());
				auto rd = rm.GetRawDataByName(r.GetName());
				auto rdata = rd.GetData();
				auto rsize = rd.GetSize();
				if (rsize > 32)
					rsize = 32;
				printf("     ");
				if (!rdata && rsize > 0)
					printf("[NO LOADING NEEDED]");
				if (rdata)
					DumpHex(rdata, rsize);
				printf("\n");

			}
	}

	BAMS::Finalize();
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	wait_for_esc();
	return 0;
}

