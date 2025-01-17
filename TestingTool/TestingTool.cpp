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

//void AddModel(BAMS::CEngine &en, uint32_t wnd, const char *repoResourceName)
//{
//	PADD_MODEL p = { wnd, repoResourceName };
//	en.SendMsg(ADD_MODEL, RENDERING_ENGINE, 0, &p);
//	return;
//}

//Properties *GetShaderParams(BAMS::CEngine &en, uint32_t wnd, const char *shaderName)
//{
//	Properties *prop;
//	PGET_SHADER_PARAMS p = { wnd, shaderName, &prop };
//	en.SendMsg(GET_SHADER_PARAMS, RENDERING_ENGINE, 0, &p);
//
//	return prop;
//}

//Properties * GetObjectParams(BAMS::CEngine &en, uint32_t wnd, const char *objectName, uint32_t objectIdx = 0)
//{
//	Properties *prop = nullptr;
//	PGET_OBJECT_PARAMS p = { wnd, objectName, objectIdx, &prop };
//	en.SendMsg(GET_OBJECT_PARAMS, RENDERING_ENGINE, 0, &p);
//
//	return prop;
//}


Property *FindProp(MProperties &prop, const char *name)
{
	for (uint32_t i = 0; i < prop.size(); ++i)
	{
		if (strcmp(prop[i].name, name) == 0)
			return &prop[i];
	}
	return nullptr;
}

Property *FindProp(const Properties &prop, const char *name)
{
	for (uint32_t i = 0; i < prop.count; ++i)
	{
		if (strcmp(prop.properties[i].name, name) == 0)
			return &prop.properties[i];
	}
	return nullptr;
}

// starting window positions
static PCREATE_WINDOW w0 = { 0, 1200, 800, 10, 20 };
static PCREATE_WINDOW w1 = { 1, 500, 500, 1310, 20 };
static PCREATE_WINDOW w2 = { 2, 500, 200, 1310, 620 };

bool wndState[3] = { false, false, false };
//vector<MProperties> onWnd[3];			// properties of 1st model on window
vector<vProperties> deferredProp(3);    // properties for deferred resolve shader (ligth positions, debugSwitch)

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
		{ 0.8f, 1, 1, 1 },
		{ 1, 0.8f, 1, 1 },
		{ 1, 1, 0.8f, 1 }
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
	{ {-ld2,  ld,  0, 0}, { 0.3f, 0.3f, 1 }, 2 * lr  },
	{ { ld2,  ld,  0, 0}, { 0.3f, 1, 0.3f }, 2 * lr  },
	{ {   0, -ld, ld, 0}, { 1, 0.3f, 0.3f }, 2 * lr  },
	{ {-ld2, -ld, ld, 0}, { 1, 1, 0.3f }, 2 * lr  },
	{ { ld2, -ld, ld, 0}, { 0.3f, 1, 1 }, 2 * lr  },

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

void SetDeferredParams(int wnd)
{
	auto pprop = &deferredProp[wnd];

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
		else if (strcmp(p.name, "samplerReflection") == 0 && p.type == Property::PT_TEXTURE)
		{
			
			BAMS::CEngine::SetTexture(wnd, &p, "cubemap_yokohama_bc3_unorm");
			//			BAMS::CResourceManager rm;
//			rm.
//			p.Set(rm.FindExisting("cubemap_space", RESID_IMAGE));
//			p.val = rm.FindExisting("cubemap_space", RESID_IMAGE);
//			slugModel.SetMeshProperty(0, "samplerColor", Property::PT_TEXTURE, 1, rm.FindExisting("SLUG_diffuse", RESID_IMAGE));
			TRACE("samplerReflection");

		}
	}
}

// ============================================================================
//
//void Spin(BAMS::CEngine &en)
//{
//	for (uint32_t i = 0; i < COUNT_OF(onWnd); ++i)
//	{
//		if (onWnd[i].size())
//		{
//			auto p = FindProp(onWnd[i][0], "model");
//			if (!p) {
//				auto newProp = GetObjectParams(en, i, nullptr, 1);
//				if (newProp)
//				{
//					onWnd[i][0] = *newProp;
//					p = FindProp(onWnd[i][0], "model");
//				}
//			}
//			if (p)
//			{
//				SetModel(*p, 0);
//				PUPDATE_DRAW_COMMANDS cw = { i };
//				en.SendMsg(UPDATE_DRAW_COMMANDS, RENDERING_ENGINE, 0, &cw);
//			}
//		}
//	}
//}
//
// ============================================================================

std::vector<IResource *>availableModels;
std::vector<CResModelDraw *> modelsOnScreen[3];

void CreateModels(BAMS::CEngine &en)
{
	BAMS::CResourceManager rm;
	availableModels.clear();
	std::vector<IResource*> unusedModels;
	std::vector<IResource*> checkMeshes;
	std::vector<bool> isTested;
	IResource *basicShader = rm.FindExisting("basic", RESID_SHADER);

	rm.Filter(RESID_IMPORTMODEL, [&](IResource *res) {
		checkMeshes.clear();
		IResource *model = nullptr;

		rm.Filter(RESID_MESH, [&](IResource *resMesh) {
			CResMesh r(resMesh);
			if (r.GetMeshSrc() == res) 
			{
				checkMeshes.push_back(resMesh);
			}
		});

		isTested.resize(checkMeshes.size());

		if (checkMeshes.size())
		{
			// check if we have already model like this:
			rm.Filter(RESID_MODEL, [&](IResource *resModel) {
				if (model)
					return;

				CResModel r(resModel);
				IResource *mesh, *shader;
				auto cnt = r.GetMeshCount();
				bool same = cnt == checkMeshes.size();
				if (!same)
					return;

				for (uint32_t i = 0; i < cnt; ++i)
					isTested[i] = false;

				for (uint32_t i = 0; i < cnt && same; ++i)
				{
					r.GetMesh(i, &mesh, &shader);
					same = false;
					for (uint32_t j = 0; j < cnt; ++j)
					{
						if (!isTested[j] && checkMeshes[j] == mesh && shader == basicShader)
						{
							isTested[j] = true;
							same = true;
							break;
						}
					}
				}
				if (same)
					model = resModel;
			});

			if (!model)
			{
				// we cant creat here new model, we are inside loop iterating resource and we can't create new one inside.
				unusedModels.push_back(res);
			}
			else 
			{
				availableModels.push_back(model);
			}

		}
	});

	// create new models
	for (auto res : unusedModels)
	{
		auto newModelResource = rm.Create(rm.GetName(res), RESID_MODEL);
		CResModel newModel(newModelResource);

		// add meshes from this model
		rm.Filter(RESID_MESH, [&](IResource *resMesh) {
			CResMesh r(resMesh);
			if (r.GetMeshSrc() == res)
				newModel.AddMesh(resMesh, basicShader);
		});

		availableModels.push_back(newModelResource);
	}
}



void SetModelMatrix(BAMS::CEngine &en, uint32_t wnd, uint32_t num, CResModelDraw * pMod)
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
	pMod->SetMatrix(&model[0][0]);

	PUPDATE_DRAW_COMMANDS cw = { wnd };
	en.SendMsg(UPDATE_DRAW_COMMANDS, RENDERING_ENGINE, 0, &cw);
}

void DeleteModels(uint32_t wnd)
{
	for (auto pMod : modelsOnScreen[wnd])
		delete pMod;
	modelsOnScreen[wnd].clear();
}

//bool strcmpext(const char *val, const char **patt, size_t count, bool caseInsesitive = true)
//{
//	decltype(_stricmp) *cmp = caseInsesitive ? _stricmp : strcmp;
//	for (size_t i = 0; i < count; ++i)
//	{
//		if (cmp(val, patt[i]) == 0)
//			return true;
//	}
//	return false;
//}
//
//Properties * PropertiesForGui(Properties *prop)
//{
//	static const char *colors[] = {
//		"baseColor", "color"
//	};
//	static const char *drag01[] = {
//		"normalMapScale"
//	};
//
//	for (auto &p : *prop)
//	{
//		switch (p.type)
//		{
//		case Property::PT_F32:
//			if ((p.count == 3 || p.count == 4) && strcmpext(p.name, colors, COUNT_OF(colors)))
//			{
//				p.guiType = PTGUI_COLOR;
//			}
//			if (p.count <= 4 && strcmpext(p.name, drag01, COUNT_OF(drag01)))
//			{
//				p.guiType = PTGUI_DRAG;
//				p.guiMin = 0.0f;
//				p.guiMax = 1.0f;
//				p.guiStep = 0.005f;
//			}
//			break;
//		}
//	}
//	return prop;
//}

void AddModelToWnd(BAMS::CEngine &en, uint32_t wnd, uint32_t modelIdx)
{
	if (modelIdx >= availableModels.size())
		return;

	BAMS::CResourceManager rm;
	CResModelDraw *pMod = new CResModelDraw(availableModels[modelIdx]);
	pMod->AddToWindow(wnd, "model"); 
	modelsOnScreen[wnd].push_back(pMod);


	// set "baseColor" & "model"
	static float colors[][4] = {
	{ 0.8f, 1, 1, 1 },
	{ 1, 0.8f, 1, 1 },
	{ 1, 1, 0.8f, 1 }
	};
	
	static float tweaks[4] = { 0, 0, 0, 0 };
	tweaks[0] = 0.05f * modelsOnScreen[wnd].size();

	pMod->SetParam("baseColor", colors[ modelsOnScreen[wnd].size() % 3 ]);
	pMod->SetParam("normalMapScale", tweaks);
	SetModelMatrix(en, wnd, static_cast<uint32_t>(modelsOnScreen[wnd].size()), pMod);
	if (wnd == 0 && modelsOnScreen[wnd].size() == 1)
	{
//		SndMsg::ShowProperties(PropertiesForGui(pMod->GetProperties(0)), "tada!");
//		PropertiesForGui(pMod->GetProperties(0));
		pMod->ShowProperties("tada!");
//		SndMsg::ShowProperties(pMod, "tada!");
	}
}

void SpinModel(BAMS::CEngine &en)
{
	return;
	for (uint32_t i = 0; i < COUNT_OF(modelsOnScreen); ++i)
	{
		if (modelsOnScreen[i].size())
		{
			SetModelMatrix(en, i, 0, modelsOnScreen[i][0]);
		}
	}
}

// ==============================================================================================

void ToggleWnd(BAMS::CEngine &en, int wnd)
{
	static PCLOSE_WINDOW cw = { 0 };
	static PCREATE_WINDOW *wn[3] = { &w0, &w1, &w2 };
	CResourceManager rm;

	cw.wnd = wnd;
	if (wndState[wnd])
	{
		DeleteModels(wnd);
		en.SendMsg(CLOSE_WINDOW, RENDERING_ENGINE, 0, &cw);
	}
	else
	{
		en.SendMsg(CREATE_WINDOW, RENDERING_ENGINE, 0, wn[wnd]);
//		deferredProp[wnd] = *en.AddMesh(wnd, nullptr, "deferred2");
		auto hMesh = en.AddMesh(wnd, nullptr, rm.FindExisting("deferred2", CResShader::GetType()));
		deferredProp[wnd] = *en.GetMeshParams(wnd, hMesh);
		SetDeferredParams(wnd);
		defaultCam.wnd = wnd;
		en.SendMsg(SET_CAMERA, RENDERING_ENGINE, 0, &defaultCam);
	}
	wndState[wnd] = !wndState[wnd];
}

void ChangeDebugView(BAMS::CEngine &en)
{
	for (auto &prop : deferredProp)
	{
		for (uint32_t i = 0; i < prop.count; ++i)
		{
			auto &p = prop.properties[i];
			if (strcmp(p.name, "debugSwitch") == 0)
			{
				auto pVal = reinterpret_cast<uint32_t *>(p.val);
				*pVal = ((*pVal) + 1) % 6;
			}
		}
	}
}

// ==============================================================================================

// ==============================================================================================

//void _OLD_AddToWnd(BAMS::CEngine &en, uint32_t wnd, uint32_t i)
//{
//	static const char *meshes[] = { "Mesh_1", "Mesh_2", "Mesh_4", "Mesh_5", "Mesh_6", "Mesh_7" };
//	static const char *colorTextures[] = {"flipper-t1-white-red",  "test", "guard1_body", "test", "test", "test" };
//	static bool meshesChecked[100] = { false };
//	static const char *colorTexturePropertyName[] = { "file", "albedo" };
//	if (!meshesChecked[i])
//	{
//		meshesChecked[i] = true;
//		CResourceManager rm;
//		auto res = rm.FindExisting(meshes[i], CResMesh::GetType());
//		assert(res != nullptr);
//		CResMesh mesh(res);
//		auto mp = mesh.GetMeshProperties(true);
//		for (auto ctn : colorTexturePropertyName)
//		{
//			auto p = FindProp(*mp, ctn);
//			if (p) 
//			{
//				rm.Filter(mesh, p, &colorTextures[i],
//					[](BAMS::IResource *res, void *local) {
//					CResource r(res);
//					*reinterpret_cast<const char **>(local) = r.GetName();
//				});
//				break;
//			}
//		}
//	}
//
//	uint32_t oid = -1;
//	Properties *pprop = nullptr;
//	PADD_MESH addMeshParams = { wnd, meshes[i], "basic", &pprop, &oid };
//
//	en.SendMsg(ADD_MESH, RENDERING_ENGINE, 0, &addMeshParams);
//
//	if (pprop) 
//	{
//		int num = static_cast<int>(onWnd[wnd].size());
//		SetParams(pprop, num);
//		onWnd[wnd].push_back(*pprop);
//		auto pTex = FindProp(*pprop, "samplerColor");
//		if (pTex)
//		{
//			PADD_TEXTURE addTexParams = { wnd, pTex->val, colorTextures[i] };
//			en.SendMsg(ADD_TEXTURE, RENDERING_ENGINE, 0, &addTexParams);
//		}
//	}
//}
//

//#define DIRECTINPUT_VERSION 0x0800
//#include <dinput.h>
//
//#pragma comment(lib, "dinput8.lib")
//#pragma comment(lib, "dxguid.lib")
//
//bool InitMouse();
//bool ReadMouse(int &xPosRelative, int &yPosRelative);
//
//int mouseX, mouseY;
//LPDIRECTINPUT8 pDI;
//LPDIRECTINPUTDEVICE8 pMouse;
//bool isInitialized = false;
//bool bImmediate = false;
//HDC last_HDC = NULL;
//
//
//bool InitMouse()
//{
//	isInitialized = false;
//	DWORD hr = DirectInput8Create(GetModuleHandle(NULL), DIRECTINPUT_VERSION,
//		IID_IDirectInput8, (VOID**)&pDI, NULL);
//	if (FAILED(hr)) return false;
//
//	hr = pDI->CreateDevice(GUID_SysMouse, &pMouse, NULL);
//	if (FAILED(hr)) return false;
//
//	hr = pMouse->SetDataFormat(&c_dfDIMouse2);
//	if (FAILED(hr)) return false;
//
//	if (!bImmediate)
//	{
//		DIPROPDWORD dipdw;
//		dipdw.diph.dwSize = sizeof(DIPROPDWORD);
//		dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
//		dipdw.diph.dwObj = 0;
//		dipdw.diph.dwHow = DIPH_DEVICE;
//		dipdw.dwData = 16; // Arbitrary buffer size
//
//		if (FAILED(hr = pMouse->SetProperty(DIPROP_BUFFERSIZE, &dipdw.diph)))
//			return false;
//	}
//
//	pMouse->Acquire();
//	isInitialized = true;
//	return true;
//}
//
//bool ReadMouse(int &xPosRelative, int &yPosRelative)
//{
//	DIMOUSESTATE2 dims2;
//	ZeroMemory(&dims2, sizeof(dims2));
//
//	DWORD hr = pMouse->GetDeviceState(sizeof(DIMOUSESTATE2),
//		&dims2);
//	if (FAILED(hr))
//	{
//		hr = pMouse->Acquire();
//		while (hr == DIERR_INPUTLOST)
//			hr = pMouse->Acquire();
//
//		// no mouse data
//		return false;
//	}
//
//	xPosRelative = dims2.lX;
//	yPosRelative = dims2.lY;
//
//	return true;
//}

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
//	if (!wndState[0] || (!onWnd[0].size() && !modelsOnScreen[0].size()))
	if (!wndState[0] || !modelsOnScreen[0].size())
		return;
	return;
	PSET_CAMERA cam = defaultCam;
	cam.wnd = 0;

	int mx = 0, my = 0;
	Tools::ReadMouse(&mx, &my);
//	ReadMouse(mx, my);

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
	CreateModels(en);
	for (bool isRunning = true; isRunning;)
	{
		// Sleep(100);
		// Spin(en);
		SpinModel(en);
		BAMS::CEngine::Update(25.0f);
//		SleepEx(5, TRUE);
		//ProcessCam(en);

		updateAllKeysScan();
		for (uint16_t i = 0; i < sizeof(currentKeyState); ++i)
		{
			if (currentKeyState[i])
			{
				switch (i) {
				case VK_ESCAPE:
//					isRunning = false;
						break;

				case VK_NUMPAD7:
				case VK_NUMPAD8:
				case VK_NUMPAD9:
					ToggleWnd(en, i - VK_NUMPAD7);
					break;

				case VK_NUMPAD1: AddModelToWnd(en, 0, 0);	break;
				case VK_NUMPAD2: AddModelToWnd(en, 0, 1);	break;
				case VK_NUMPAD3: AddModelToWnd(en, 0, 2);	break;
				case VK_NUMPAD4: AddModelToWnd(en, 0, 3);	break;
				case VK_NUMPAD5: AddModelToWnd(en, 0, 4);	break;
				case VK_NUMPAD6: AddModelToWnd(en, 0, 5);	break;
				case VK_NUMPAD0: ChangeDebugView(en);	break;
//				case VK_ADD: AddModelToWnd(en, 0, 0);

//				default:					TRACE("KEY: " << i << "\n");
				}
			}
		}
		isRunning = false;
		for (auto w : wndState)
		{
			if (w) 
			{
				isRunning = true;
				break;
			}
		}
	}
}

template<typename T>
void PrintInfo(const char *tn)
{
	TRACE(tn << ":\n");
#define III(x)	TRACE("  "<< #x << ": " <<( std::x<T>::value ? "true\n":"false\n"))

	III(is_trivially_move_constructible);
	
	III(is_trivially_constructible);
	III(is_trivially_destructible);
	III(is_trivially_copy_constructible);
}

void DoLocalTests()
{
	//BAMS::STR s = "ala ma kota";

	//auto x = s.c_str();
	//TRACE(x);
	//using BAMS::array;
	//struct obj {
	//	int x, y;
	//	obj(int x, int y) { this->x = x; this->y = y; }

	//};
	//array<obj> ta;
	//PrintInfo<obj>("obj");
	//ta.emplace_back( 1,2 );
	//ta.emplace_back( 2,3 );
	//ta.emplace_back( 3,4 );

	//TRACE("ta: " << ta._reserved << ":" << ta._used << ":" << ta._storage << "\n");

	//struct ob2 {
	//	int x;
	//	std::string s;
	//	ob2(int x, const char *s) 
	//	{ 
	//		this->x = x; this->s = s; 
	//	}
	//};

	//ob2 hmm(5, "sazdfasedf");

	//PrintInfo<std::string>("ob2");
	//array<ob2> t2;
	//PrintInfo<ob2>("ob2");
	//t2.emplace_back(1, "haha");
	//t2.emplace_back(2, "haha2");
	//t2.emplace_back(3, "ala ma kota");
	//TRACE("t2: " << t2._reserved << ":" << t2._used << ":" << t2._storage << "\n");

	//struct ob3 {
	//	int x;
	//	const char * s;
	//	ob3(int x, const char *s)
	//	{
	//		auto l = strlen(s)+1;
	//		auto buf = new char[l];
	//		memcpy_s(buf, l, s, l);
	//		this->x = x; 
	//		this->s = buf;
	//	}

	//	// copy constructor
	//	ob3(const ob3 &src)
	//	{
	//		x = src.x;
	//		auto l = strlen(src.s) + 1;
	//		auto buf = new char[l];
	//		memcpy_s(buf, l, src.s, l);
	//		this->x = x;
	//		this->s = buf;
	//	}

	//	// move constructor
	//	ob3(ob3 &&src) noexcept
	//	{
	//		x = src.x;
	//		s = src.s;
	//		src.s = nullptr;
	//	}

	//	ob3& operator = (ob3 && src) noexcept
	//	{
	//		x = src.x;
	//		s = src.s;
	//		src.s = nullptr;
	//		return *this;
	//	}

	//	~ob3()
	//	{
	//		if (s)
	//			delete s;
	//	}
	//};

	//std::vector<ob3> t4;
	//t4.emplace_back(1, "haha");
	//t4.emplace_back(2, "haha2");
	//t4.emplace_back(3, "ala ma kota");


	//array<ob3> t3;
	//PrintInfo<ob3>("ob3");
	//t3.emplace_back(1, "haha");
	//t3.emplace_back(2, "haha2");
	//t3.emplace_back(3, "ala ma kota");
	//TRACE("t3: " << t3._reserved << ":" << t3._used << ":" << t3._storage << "\n");

}

int main()
{
	uint64_t Max, Current, Counter;

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);
	
	BAMS::Initialize(); // Starts Game Engine Thread, it will create all needed threads ...
	LoadModules();
	{
		BAMS::CEngine en;
		//		DoLocalTests();

		BAMS::DoTests();
		BAMS::CResourceManager rm;


		rm.RootDir(L"C:\\Work\\test");
		rm.LoadSync();

		// create ui shader
		CResShader gs = rm.Get<CResShader>("__imgui__");
		gs.AddProgram(L"/Shaders/imgui/imgui.vert.glsl");
		gs.AddProgram(L"/Shaders/imgui/imgui.frag.glsl");
		// slug textures fix:
		CResModel slugModel = rm.Get<CResModel>("slug2");
		slugModel.SetMeshProperty(0, "samplerColor", Property::PT_TEXTURE, 1, rm.FindExisting("SLUG_diffuse", RESID_IMAGE));
		slugModel.SetMeshProperty(0, "samplerNormal", Property::PT_TEXTURE, 1, rm.FindExisting("SLUG_normal", RESID_IMAGE));

		ToggleWnd(en, 0); // show first window


		CResImage ri = rm.FindExisting("test", RESID_IMAGE);
		auto img = ri.GetImage(true);

		auto resmesh = rm.FindExisting("Mesh_1", RESID_MESH);
		if (resmesh) {
			CResMesh m1(resmesh);
			auto vd = m1.GetVertexDescription(true);
		}

		auto tt = rm.FindOrCreate(L"C:\\Work\\BPS\\BAMEngine\\ReadMe.txt", RESID_RAWDATA);

		auto x = rm.FindExisting("deferred2", CResShader::GetType());
		if (x) {
			TRACE("--- panic ---");
		}
		// default shader program ... not needed any more
		if (!rm.FindExisting("deferred", CResShader::GetType())) {
			auto s = rm.Get<CResShader>("deferred");
			s.AddProgram(L"/Shaders/deferred/deferred.vert.glsl");
			s.AddProgram(L"/Shaders/deferred/deferred.frag.glsl");
		}
		//			auto s = rm.GetShaderByName("default");
//			s.AddProgram(L"/Shaders/default.vert.glsl");
//			s.AddProgram(L"/Shaders/default.frag.glsl");

		auto r = rm.Get<CResRawData>("ReadMe");
		printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
		rm.LoadSync();
		printf("ReadMe is loade? %s\n", r.IsLoaded() ? "yes" : "no");
		auto ruid = r.GetUID();
		auto rpath = r.GetPath();
		auto rname = r.GetName();
		auto rdata = r.GetData();
		auto rsize = r.GetSize();

		CResource res = rm.FindOrCreate("ReadMe");
		auto uid = res.GetUID();
		auto path = res.GetPath();
		auto name = res.GetName();

		auto rr = rm.Get<CResRawData>(uid);
		uid = rr.GetUID();
		path = rr.GetPath();
		name = rr.GetName();


		// loop
		testloop(en);

		rm.LoadSync();
	}

	// remove models before call to Finalize 
	for (uint32_t wnd : {0, 1, 2})
		DeleteModels(wnd);

	deferredProp.clear();

	BAMS::Finalize();

	BAMS::GetMemoryAllocationStats(&Max, &Current, &Counter);

	DumpRAM();
	wait_for_esc();
	return 0;
}

