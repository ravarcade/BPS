// TestingTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BAMEngine.h"
#include <chrono>
#include "../3rdParty/glm/glm.hpp"
#include "../3rdParty/glm/gtc/matrix_transform.hpp"

using namespace BAMS;

void LoadModule(const wchar_t *module)
{
	auto hm = LoadLibrary(module);

	typedef void func(void);

	func *RegisterModule = (func *)GetProcAddress(hm, "RegisterModule");
	if (RegisterModule)
		RegisterModule();
}

void LoadModules()
{
	LoadModule(L"RenderingEngineVK.dll");
	LoadModule(L"ImportModule.dll");
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

void MemStat()
{
	uint64_t Max, Current, Counter;
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	printf("==================== Max = %8d, Current = %8d, Count = %8d ====================\n", (int)Max, (int)Current, (int)Counter);
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

	MemStat();
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

BAMS::CORE::Properties *GetShaderParams(BAMS::CEngine &en, uint32_t wnd, const char *shaderName)
{
	BAMS::CORE::Properties *prop;
	PGET_SHADER_PARAMS p = { wnd, shaderName, &prop };
	en.SendMsg(GET_SHADER_PARAMS, RENDERING_ENGINE, 0, &p);

	return prop;
}

BAMS::CORE::Properties * GetObjectParams(BAMS::CEngine &en, uint32_t wnd, const char *objectName)
{
	BAMS::CORE::Properties *prop;
	PGET_OBJECT_PARAMS p = { wnd, objectName, &prop };
	en.SendMsg(GET_OBJECT_PARAMS, RENDERING_ENGINE, 0, &p);

	return prop;
}


static PCREATE_WINDOW w0 = { 0, 1200, 800, 10, 20 };
static PCREATE_WINDOW w1 = { 1, 500, 500, 1310, 20 };
static PCREATE_WINDOW w2 = { 2, 500, 200, 1310, 620 };

using BAMS::CORE::MProperties;
using BAMS::CORE::Properties;
std::vector<MProperties> onWnd[3];
bool wndState[3] = { true, false, false };

uint32_t propIdxModel = -1;
uint32_t propIdxBaseColor = -1;

void SetPropIdx(Properties *pprop)
{
	static bool once = true;
	if (once) {
		once = false;
		for (uint32_t i = 0; i<pprop->count; ++i)
		{
			auto &p = pprop->properties[i];
			if (strcmp(p.name, "model") == 0)
				propIdxModel = i;
			if (strcmp(p.name, "baseColor") == 0)
				propIdxBaseColor = i;
		}
	}
}

void SetObjParams(Properties *pprop, int num)
{
	static float colors[][4] = {
		{ 0.1f, 1, 1, 1 },
		{ 1, 0.1f, 1, 1 },
		{ 1, 1, 0.1f, 1 }
	};
	auto &b = pprop->properties[propIdxBaseColor];
	auto &m = pprop->properties[propIdxModel];
	memcpy_s(b.val, 4 * sizeof(float), colors[num % 3], 4 * sizeof(float));


	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	uint32_t i = num;
	int pos = ((i & 1) ? 1 : -1) * ((i + 1) & 0xfffe);
//	static float s = 0.02f;
	static float s = 1.0f;
	glm::mat4 I(1.0f);
	auto S = glm::scale(I, glm::vec3(s));
//	auto R = glm::rotate(I, time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	auto R = glm::rotate(I, pos * glm::radians(90.0f/4), glm::vec3(0.0f, 0.0f, 1.0f));
	auto T = glm::translate(I, glm::vec3(pos * 15, 0, 0));
	auto model = T * R * S;
	
	memcpy_s(m.val, 16 * sizeof(float), &model[0][0], 16 * sizeof(float));

}

void Spin(BAMS::CEngine &en)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	for (uint32_t i = 0; i < COUNT_OF(onWnd); ++i)
	{
		auto &wnd = onWnd[i];
		if (wnd.size())
		{
			static float s = 1.0f;
			auto &prop = wnd[0];
			auto &m = prop[propIdxModel];
			glm::mat4 I(1.0f);
			auto S = glm::scale(I, glm::vec3(1,1,0.1));
			auto R = glm::rotate(I, time * glm::radians(90.0f / 4), glm::vec3(0.0f, 0.0f, 1.0f));
			auto T = glm::translate(I, glm::vec3(0, 0, 0));
			auto model = T * R * S;
			TRACEM(model);
			auto m2 = glm::transpose(glm::inverse(model));
			TRACEM(m2);

			memcpy_s(m.val, 16 * sizeof(float), &model[0][0], 16 * sizeof(float));
			PUPDATE_DRAW_COMMANDS cw = { i };
			en.SendMsg(UPDATE_DRAW_COMMANDS, RENDERING_ENGINE, 0, &cw);
		}
	}
}

void AddToWnd(BAMS::CEngine &en, int wnd, int i)
{
	static PADD_MESH c[] = {
//		{ 0, "realcubename", "Mesh_1", "basic" },
		{ 0, "realcubename", "Mesh_10", "basic" },
		{ 0, "cubename", "Mesh_2", "basic" },
		{ 0, "realcubename", "Mesh_4", "basic" },
		{ 0, "realcubename", "Mesh_5", "basic" },
		{ 0, "realcubename", "Mesh_6", "basic" },
		{ 0, "realcubename", "Mesh_7", "basic" },
	};
	uint32_t oid = -1;
	Properties *pprop = nullptr;
	PADD_MESH *obj = &c[i];
	obj->wnd = wnd;
	obj->pProperties = &pprop;
	obj->pId = &oid;
	en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, obj);
	if (pprop) {
		SetPropIdx(pprop);
		SetObjParams(pprop, (int)onWnd[wnd].size());
		onWnd[wnd].push_back(*pprop);
	}
}

void TogleWnd(BAMS::CEngine &en, int wnd)
{
	static PCLOSE_WINDOW cw = { 0 };
	static PCREATE_WINDOW *wn[3] = { &w0, &w1, &w2 };
	cw.wnd = wnd;
	if (wndState[wnd])
	{
		en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw);
		onWnd[wnd].clear();
	}
	else
	{
		en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, wn[wnd]);
	}
	wndState[wnd] = !wndState[wnd];
}

void testloop(BAMS::CEngine &en)
{
	for (bool isRunning = true; isRunning;)
	{
		//		Sleep(100);
		Spin(en);
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
					en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, nullptr);
					break;

				case '1':
				case '2':
				case '3':
					TogleWnd(en, i - '1');
					break;

				case 'Q': AddToWnd(en, 0, 0);	break;
				case 'A': AddToWnd(en, 0, 1);	break;
				case 'W': AddToWnd(en, 1, 2);	break;
				case 'S': AddToWnd(en, 1, 3);	break;
				case 'E': AddToWnd(en, 2, 4);	break;
				case 'D': AddToWnd(en, 2, 5);	break;
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
	LoadModules();
	{
		BAMS::CEngine en;


		BAMS::DoTests();
			BAMS::CResourceManager rm;
			rm.RootDir(L"C:\\Work\\test");
			rm.LoadSync();
			en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, &w0, sizeof(w0));


			CMesh m1 = rm.Find("Mesh_1", RESID_MESH);
			auto vd = m1.GetVertexDescription();
			rm.AddResource(L"C:\\Work\\BPS\\BAMEngine\\ReadMe.txt");
			
			// default shader program ... not needed any more
//			auto s = rm.GetShaderByName("default");
//			s.AddProgram(L"/Shaders/default.vert.glsl");
//			s.AddProgram(L"/Shaders/default.frag.glsl");

			auto r = rm.GetRawData("ReadMe");
			printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
			rm.LoadSync();
			printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
			auto ruid = r.GetUID();
			auto rpath = r.GetPath();
			auto rname = r.GetName();
			auto rdata = r.GetData();
			auto rsize = r.GetSize();

			CResource res = rm.Find("ReadMe");
			auto uid = res.GetUID();
			auto path = res.GetPath();
			auto name = res.GetName();

			auto rr = rm.GetRawData(uid);
			uid = rr.GetUID();
			path = rr.GetPath();
			name = rr.GetName();


			// loop
			testloop(en);

			rm.LoadSync();

			uint32_t resCount = 0;
			rm.Filter([](IResource *res, void *prm) {

				BAMS::CResourceManager &rm = *static_cast<BAMS::CResourceManager *>(prm);
				BAMS::CResource r(res);
				if (!r.IsLoaded())
					rm.LoadSync();
				printf("%3d: \"%s\", %#010x, %s", 0, r.GetName(), r.GetType(), UUID2String(r.GetUID()));
				wprintf(L", \"%s\"\n", r.GetPath());
				if (r.IsLoadable())
				{
					auto rd = rm.GetRawData(r.GetName());
					auto rdata = rd.GetData();
					auto rsize = rd.GetSize();
					if (rsize > 32)
						rsize = 32;
					printf("     ");
					if (!rdata && rsize > 0)
						printf("[NO LOADING NEEDED]");
					if (rdata)
						DumpHex(rdata, rsize);
				}
				else {
					printf("[NO LOADING NEEDED]");
				}
				printf("\n");
			}, &rm);
	}

	BAMS::Finalize();
	for (auto &w : onWnd)
		w.clear();
	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	wait_for_esc();
	return 0;
}

