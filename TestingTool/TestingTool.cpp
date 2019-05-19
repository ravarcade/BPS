// TestingTool.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "BAMEngine.h"
#include <chrono>
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

#include <vector>
#pragma comment(lib, "Winmm.lib")

using namespace BAMS;
using std::vector;

extern PSET_CAMERA defaultCam;

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
		static char txt[2] = { 0, 0 };
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

BYTE AllKeyboardStates[256] = { 0 };
DWORD AllKeyboardRepeatTime[256];
BYTE currentKeyState[256];
BYTE KeyStates[256];

void updateAllKeysScan()
{
	const DWORD rep_first = 300;
	const DWORD rep_next = 50;

	DWORD ct = timeGetTime();
	GetKeyboardState(KeyStates);

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

bool IsKeyPressed(DWORD key)
{
	return KeyStates[key] & 0x80;
}
// ====================================================================

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

// ==================================================================

void wait_for_esc()
{
	for (;;)
	{
		BAMS::CEngine::Update(25.0f);
		SleepEx(25, TRUE);
		updateAllKeysScan();
		if (GetKeyPressed(VK_ESCAPE))
			break;

	}
}

// ================================================================= 

Properties *GetShaderParams(BAMS::CEngine &en, uint32_t wnd, const char *shaderName)
{
	Properties *prop;
	PGET_SHADER_PARAMS p = { wnd, shaderName, &prop };
	en.SendMsg(GET_SHADER_PARAMS, RENDERING_ENGINE, 0, &p);

	return prop;
}

Properties * GetObjectParams(BAMS::CEngine &en, uint32_t wnd, const char *objectName)
{
	Properties *prop;
	PGET_OBJECT_PARAMS p = { wnd, objectName, &prop };
	en.SendMsg(GET_OBJECT_PARAMS, RENDERING_ENGINE, 0, &p);

	return prop;
}


Property *FindProp(MProperties &prop, const char *name)
{
	for (uint32_t i = 0; i < prop.size(); ++i)
	{
		if (strcmp(prop[i].name, name) == 0)
			return &prop[i];
	}
	return nullptr;
}

// starting window positions
static PCREATE_WINDOW w0 = { 0, 1200, 800, 10, 20 };
static PCREATE_WINDOW w1 = { 1, 500, 500, 1310, 20 };
static PCREATE_WINDOW w2 = { 2, 500, 200, 1310, 620 };

bool wndState[3] = { false, false, false };
vector<MProperties> onWnd[3];			// properties of 1st model on window
vector<MProperties> deferredProp(3);    // properties for deferred resolve shader (ligth positions, debugSwitch)

void SetModel(Property &p, uint32_t num)
{
	static auto startTime = std::chrono::high_resolution_clock::now();
	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	uint32_t i = num;
	int pos = ((i & 1) ? 1 : -1) * ((i + 1) & 0xfffe);

	static float s = 1.0f;
	glm::mat4 I(1.0f);
	auto S = glm::scale(I, glm::vec3(s));
	auto R = glm::rotate(I, time * glm::radians(90.0f / 4), glm::vec3(0.0f, 0.0f, 1.0f));
	auto T = glm::translate(I, glm::vec3(pos * 15, 0, 0));
	auto model = T * R * S;

	memcpy_s(p.val, 16 * sizeof(float), &model[0][0], 16 * sizeof(float));
}

void SetBaseColor(Property &p, uint32_t num)
{
	static float colors[][4] = {
		{ 0.5f, 1, 1, 1 },
		{ 1, 0.5f, 1, 1 },
		{ 1, 1, 0.5f, 1 }
	};

	memcpy_s(p.val, 4 * sizeof(float), colors[num % 3], 4 * sizeof(float));
}

void SetParams(Properties *pprop, uint32_t num)
{
	for (uint32_t i = 0; i < pprop->count; ++i)
	{
		auto &p = pprop->properties[i];
		if (strcmp(p.name, "model") == 0)
		{
			SetModel(p, num);
		}
			
		else if (strcmp(p.name, "baseColor") == 0)
		{
			SetBaseColor(p, num);
		}
	}
}

const float lr = 1350;
const float ld = 100;// lr * 0.5;
const float ld2 = 100;// lr * 0.3;

struct Light {
	float position[4];
	float color[3];
	float radius;
} GlobalLights[6] = {
	{ {   0,  ld,  0, 0}, { 1, 1, 1 }, 2 * lr },
	{ {-ld2,  ld,  0, 0}, { 0.3, 0.3, 1 }, 2 * lr  },
	{ { ld2,  ld,  0, 0}, { 0.3, 1, 0.3 }, 2 * lr  },
	{ {   0, -ld, ld, 0}, { 1, 0.3, 0.3 }, 2 * lr  },
	{ {-ld2, -ld, ld, 0}, { 1, 1, 0.3 }, 2 * lr  },
	{ { ld2, -ld, ld, 0}, { 0.3, 1, 1 }, 2 * lr  },

};


void SetLightPosition(Property &p, uint32_t i, uint32_t stride)
{
	memcpy_s(
		reinterpret_cast<uint8_t *>(p.val) + i * stride, 
		4 * sizeof(float), 
		GlobalLights[i].position,
		4 * sizeof(float));
}

void SetLightColor(Property &p, uint32_t i, uint32_t stride)
{
	memcpy_s(
		reinterpret_cast<uint8_t *>(p.val) + i * stride,
		3 * sizeof(float),
		GlobalLights[i].color,
		3 * sizeof(float));
}


void SetLightRadius(Property &p, uint32_t i, uint32_t stride)
{
	memcpy_s(
		reinterpret_cast<uint8_t *>(p.val) + i * stride,
		1 * sizeof(float),
		&GlobalLights[i].radius,
		1 * sizeof(float));
}

const float deg2rad = 0.01745329251994329576923690768489f;

void SetDeferredParams(Properties *pprop)
{
	uint32_t stride = 0;
	uint32_t count = 0;
	float n = defaultCam.zNear;
	float f = defaultCam.zFar;
	for (uint32_t i = 0; i < pprop->count; ++i)
	{
		auto &p = pprop->properties[i];
		if (strcmp(p.name, "lights") == 0 && p.type == Property::PT_ARRAY)
		{
			stride = p.array_stride;
			count = p.count;
		}
		else if (strcmp(p.name, "position") == 0)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				SetLightPosition(p, i, stride);
			}
		}
		else if (strcmp(p.name, "color") == 0)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				SetLightColor(p, i, stride);
			}
		}
		else if (strcmp(p.name, "radius") == 0)
		{
			for (uint32_t i = 0; i < count; ++i)
			{
				SetLightRadius(p, i, stride);
			}
		}
	}
}


void Spin(BAMS::CEngine &en)
{
	for (uint32_t i = 0; i < COUNT_OF(onWnd); ++i)
	{
		if (onWnd[i].size())
		{
			auto p = FindProp(onWnd[i][0], "model");
			if (p)
			{
				SetModel(*p, 0);
				PUPDATE_DRAW_COMMANDS cw = { i };
				en.SendMsg(UPDATE_DRAW_COMMANDS, RENDERING_ENGINE, 0, &cw);
			}
		}
	}
}

void AddToWnd(BAMS::CEngine &en, uint32_t wnd, uint32_t i)
{
	static const char *meshes[] = { "Mesh_1", "Mesh_2", "Mesh_4", "Mesh_5", "Mesh_6", "Mesh_7" };
	uint32_t oid = -1;
	Properties *pprop = nullptr;
	PADD_MESH addMeshParams = { wnd, "name is irrelevant", meshes[i], "basic", &pprop, &oid };

	en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, &addMeshParams);

	if (pprop) 
	{
		int num = static_cast<int>(onWnd[wnd].size());
		SetParams(pprop, num);
		onWnd[wnd].push_back(*pprop);

		PADD_TEXTURE addTexParams = { wnd, oid, 0, "test" };
		en.SendMsg(ADD_TEXTURE, RENDERING_ENGINE, 0, &addTexParams);	
	}
}

void ToggleWnd(BAMS::CEngine &en, int wnd)
{
	static PCLOSE_WINDOW cw = { 0 };
	static PCREATE_WINDOW *wn[3] = { &w0, &w1, &w2 };
	Properties *pprop = nullptr;
	PADD_MESH deferredResolveShader = { 0, "", "", "deferred", &pprop, nullptr };

	cw.wnd = wnd;
	if (wndState[wnd])
	{
		en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw);
		onWnd[wnd].clear();
	}
	else
	{
		en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, wn[wnd]);
		deferredResolveShader.wnd = wnd;
		en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, &deferredResolveShader);
		deferredProp[wnd] = *pprop;
		SetDeferredParams(pprop);
		defaultCam.wnd = wnd;
		en.SendMsg(SET_CAMERA, RENDERING_ENGINE, 0, &defaultCam);
	}
	wndState[wnd] = !wndState[wnd];
}

void ChangeDebugView(BAMS::CEngine &en)
{
	for (MProperties &prop : deferredProp)
	{
		for (uint32_t i = 0; i < prop.count; ++i)
		{
			auto &p = prop.properties[i];
			if (strcmp(p.name, "debugSwitch") == 0 )
			{
				auto pVal = reinterpret_cast<uint32_t *>(p.val);
				*pVal = ((*pVal) + 1) % 6;
			}
		}
	}
}

// ========================================================================
#define DIRECTINPUT_VERSION 0x0800
#include <dinput.h>

#pragma comment(lib, "dinput8.lib")
#pragma comment(lib, "dxguid.lib")

bool InitMouse();
bool ReadMouse(int &xPosRelative, int &yPosRelative);

int mouseX, mouseY;
LPDIRECTINPUT8 pDI;
LPDIRECTINPUTDEVICE8 pMouse;
bool isInitialized = false;
bool bImmediate = false;
HDC last_HDC = NULL;


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

bool ReadMouse(int &xPosRelative, int &yPosRelative)
{
	DIMOUSESTATE2 dims2;
	ZeroMemory(&dims2, sizeof(dims2));

	DWORD hr = pMouse->GetDeviceState(sizeof(DIMOUSESTATE2),
		&dims2);
	if (FAILED(hr))
	{
		hr = pMouse->Acquire();
		while (hr == DIERR_INPUTLOST)
			hr = pMouse->Acquire();

		// no mouse data
		return false;
	}

	xPosRelative = dims2.lX;
	yPosRelative = dims2.lY;

	return true;
}

// ========================================================================
// Camera
PSET_CAMERA defaultCam = {
	0, // first window
	{ 0.0f, 100.0f, 0.0f },
	{ 0, 0, 0 },
	{ 0, 0, 1},
	45.0f,   // fov
	1.0f,    // z-near
	1000.0f, // z-far
};

const float mouseScale = 0.001f;
const float moveScale = 5.0f;

void ProcessCam(CEngine &en)
{
	if (!wndState[0] || !onWnd[0].size())
		return;

	PSET_CAMERA cam = defaultCam;
	cam.wnd = 0;

	int mx = 0, my = 0;
	ReadMouse(mx, my);

	static glm::vec3 static_target(
		defaultCam.lookAt[0] - defaultCam.eye[0],
		defaultCam.lookAt[1] - defaultCam.eye[1],
		defaultCam.lookAt[2] - defaultCam.eye[2]);

	static glm::vec3 rot(0);
	static glm::vec3 pos(
		defaultCam.eye[0],
		defaultCam.eye[1],
		defaultCam.eye[2]);

	static_target = glm::normalize(static_target);
	static_target = glm::vec3(0, -1, 0);
	rot.z -= float(mx)*mouseScale;
	rot.x += float(my)*mouseScale;

	glm::mat3 r = glm::mat3_cast(glm::quat(rot));
	auto lookAt = r * static_target;
	glm::vec3 right = r * glm::vec3(-1, 0, 0);

	if (IsKeyPressed('W'))
	{
		pos += lookAt * moveScale;
	}
	if (IsKeyPressed('S'))
	{
		pos -= lookAt * moveScale;
	}
	if (IsKeyPressed('A'))
	{
		pos -= right * moveScale;
	}
	if (IsKeyPressed('D'))
	{
		pos += right * moveScale;
	}
	if (GetKeyPressed('R'))
	{
		pos = glm::vec3(
			defaultCam.eye[0],
			defaultCam.eye[1],
			defaultCam.eye[2]);
		rot = glm::vec3(0);
	}

	cam.eye[0] = pos.x;
	cam.eye[1] = pos.y;
	cam.eye[2] = pos.z;
	cam.lookAt[0] = cam.eye[0] + lookAt.x;
	cam.lookAt[1] = cam.eye[1] + lookAt.y;
	cam.lookAt[2] = cam.eye[2] + lookAt.z;
	en.SendMsg(SET_CAMERA, RENDERING_ENGINE, 0, &cam);
}

// ========================================================================
void testloop(BAMS::CEngine &en)
{
	for (bool isRunning = true; isRunning;)
	{
		//		Sleep(100);
		Spin(en);
		BAMS::CEngine::Update(25.0f);
		SleepEx(25, TRUE);
		ProcessCam(en);

		updateAllKeysScan();
		for (uint16_t i = 0; i < sizeof(currentKeyState); ++i)
		{
			if (currentKeyState[i])
			{
				switch (i) {
				case VK_ESCAPE:
					isRunning = false;
						break;

				case VK_NUMPAD7:
				case VK_NUMPAD8:
				case VK_NUMPAD9:
					ToggleWnd(en, i - VK_NUMPAD7);
					break;

				case VK_NUMPAD4: AddToWnd(en, 0, 0);	break;
				case VK_NUMPAD1: AddToWnd(en, 0, 1);	break;
				case VK_NUMPAD5: AddToWnd(en, 1, 2);	break;
				case VK_NUMPAD2: AddToWnd(en, 1, 3);	break;
				case VK_NUMPAD6: AddToWnd(en, 2, 4);	break;
				case VK_NUMPAD3: AddToWnd(en, 2, 5);	break;
				case VK_NUMPAD0: ChangeDebugView(en);	break;

//				default:					TRACE("KEY: " << i << "\n");
				}
			}
		}
	}
}

int main()
{
	InitMouse();
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
			ToggleWnd(en,0); // show first window

			CResImage ri = rm.Find("test", RESID_IMAGE);
			auto img = ri.GetImage(true);

			CResMesh m1 = rm.Find("Mesh_1", RESID_MESH);
			auto vd = m1.GetVertexDescription(true);
			rm.AddResource(L"C:\\Work\\BPS\\BAMEngine\\ReadMe.txt");
			
			// default shader program ... not needed any more
			auto s = rm.GetShaderByName("deferred");
			s.AddProgram(L"/Shaders/deferred/deferred.vert.glsl");
			s.AddProgram(L"/Shaders/deferred/deferred.frag.glsl");
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
	deferredProp.clear();

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	wait_for_esc();
	return 0;
}

