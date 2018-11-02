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

void VkDoTests()
{
	CAttribStreamConverter cv;
	float vert[1024 * 4];
	uint32_t mid[1024];
	uint32_t mid2[1024];
	float out[1024 * 4];
	// fill
	for (int i = 0; i < 1024; ++i)
	{
		vert[i * 4 + 0] = (i - 512) / float(0x1ff);
		vert[i * 4 + 1] = (i - 512) / float(0x1ff);;
		vert[i * 4 + 2] = (i - 512) / float(0x1ff);;
		vert[i * 4 + 3] = float((i % 4) - 2);
	}

	Stream src((uint32_t)::BAMS::RENDERINENGINE::FLOAT_4D, 16, false);
	src.m_data = vert;
	Stream mi((uint32_t)::BAMS::RENDERINENGINE::INT_PACKED, 4, true);
	mi.m_data = mid;
	Stream ou((uint32_t)::BAMS::RENDERINENGINE::FLOAT_4D, 16, false);
	ou.m_data = out;
	Stream mi2((uint32_t)::BAMS::RENDERINENGINE::INT_PACKED, 4, true);
	mi2.m_data = mid2;

	cv.Convert(mi, src, 1024);
	cv.Convert(ou, mi, 1024);
	cv.Convert(mi2, ou, 1024);

	for (int i = 0; i < 1024; ++i)
	{
		if (i > 0 && i < 1023 && (mid[i] == mid[i - 1] || mid[i] == mid[i + 1]))
			printf("converr: %d\n", i);
		if (mid[i] != mid2[i])
			printf("diff: %d\n", i);
	}
	for (int i = 0; i < 1024; ++i)
	{
		if (vert[i * 4 + 0] != out[i * 4 + 0])
		{
			printf("dif: %d\n", i);
		}
		if (vert[i * 4 + 1] != out[i * 4 + 1])
		{
			printf("dif: %d\n", i);
		}
		if (vert[i * 4 + 2] != out[i * 4 + 2])
		{
			printf("dif: %d\n", i);
		}

		if (vert[i * 4 + 3] != out[i * 4 + 3])
		{
			printf("dif: %d\n", i);
		}
	}
}

// ============================================================================

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
		case CREATE_3D_WINDOW: {
			re.CreateWnd(MAINWND, nullptr);
			re.CreateWnd(BACKBOXWND, nullptr);
			re.CreateWnd(DMDWND, nullptr);
			//			VkDoTests();
			auto params = reinterpret_cast<const RenderingModel *>(msg->data);
//			re.Add3DModel(MAINWND, params);
//			re.Add3DModel(BACKBOXWND, params);
//			re.Add3DModel(DMDWND, params);
		}
			break;

		case ADD_3D_MODEL: {
			auto params = reinterpret_cast<const RenderingModel *>(msg->data);
//			re.Add3DModel(MAINWND, params);
//			re.Add3DModel(BACKBOXWND, params);
//			re.Add3DModel(DMDWND, params);
		} break;

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