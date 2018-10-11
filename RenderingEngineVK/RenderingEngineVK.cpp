// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "BAMEngine.h"

using namespace BAMS;

#pragma pack(push,1)
struct Point3D { float x, y, z; };
struct Point2D { float x, y; };

struct Model 
{
	uint32_t noVertices;
	uint32_t noFaces;
	Point3D *vertices;
	Point3D *normals;
	Point2D *textCoord;
	uint32_t *faces;
};

#pragma pack(pop)

Point3D defaultBoxVertices[] = {
	{ -1, -1, -1 },
	{  1, -1, -1 },
	{  1,  1, -1 },
	{ -1,  1, -1 },

	{ -1, -1,  1 },
	{  1, -1,  1 },
	{  1,  1,  1 },
	{ -1,  1,  1 }
};

uint32_t defaultBoxFaces[] = {
	0, 1, 2, 2, 3, 0,
	0, 4, 5, 1, 0, 5,
};

Model defaultBox = {
	8, 16,
	defaultBoxVertices,
	nullptr,
	nullptr,
	defaultBoxFaces
};

// ============================================================================

void doTests();
/// <summary>
///  
/// </summary>
/// <seealso cref="IExternalModule{RenderingModule, RENDERING_ENGINE}" />
class RenderingModule : public IExternalModule<RenderingModule, RENDERING_ENGINE>
{

public:
	RenderingModule() {}

	void Initialize() 
	{ 
		glfw.Init();
		re.Init();
	}

	void Finalize() { 
		re.Cleanup();
		glfw.Cleanup();
	}

	void Update(float dt) 
	{ 
		glfw.Update(dt);
		re.Update(dt);
	}

	void SendMsg(Message *msg) 
	{ 
		switch (msg->id)
		{
		case CREATE_3D_WINDOW:
		{
			//int width = 800, height = 600;
			//glfw.CreateWnd(MAINWND, width, height);
//			re.Create_main3dwindow();
			doTests();
			re.CreateWnd(MAINWND, nullptr);
			re.CreateWnd(BACKBOXWND, nullptr);
			re.CreateWnd(DMDWND, nullptr);
		}
			break;
		}
	}

	void Destroy() {}
};


RenderingModule RM;

extern "C" {
	REVK_EXPORT void RenderingEngine_RegisterModule()
	{
		printf("HELLO REVK\n");
		RM.Register();
		
	}

}