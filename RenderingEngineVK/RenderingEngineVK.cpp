// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "BAMEngine.h"

using namespace BAMS;

BAMS::RENDERINENGINE::VertexDescription *GetDemoCube()
{
	using namespace BAMS::RENDERINENGINE;

	// demo cube
	struct Vertex {
		glm::vec3 pos;
		DWORD color;
	};

	static const std::vector<Vertex> vertices = {
		{ { -0.5f, -0.5f,  0.5f }, 0x0000000ff },
		{ { 0.5f, -0.5f,  0.5f },  0x0000ff00 },
		{ { 0.5f,  0.5f,  0.5f },  0x00ff0000},
		{ { -0.5f,  0.5f,  0.5f }, 0x00ffffff },

		{ { -0.5f, -0.5f, -0.5f }, 0x00ff0000 },
		{ { 0.5f, -0.5f, -0.5f },  0x00ffffff },
		{ { 0.5f,  0.5f, -0.5f },  0x000000ff },
		{ { -0.5f,  0.5f, -0.5f }, 0x0000ff00 }
	};

	static const std::vector<uint16_t> indices = {
		0, 1, 2,  2, 3, 0,
		0, 4, 5,  1, 0, 5,
		1, 5, 6,  2, 1, 6,
		2, 6, 7,  3, 2, 7,
		3, 7, 4,  0, 3, 4,
		5, 4, 6,  7, 6, 4
	};
	static VertexDescription vd;
	static bool initOnce = true;
	if (initOnce) {
		vd.m_numVertices = static_cast<U32>(vertices.size());
		vd.m_numIndices = static_cast<U32>(indices.size());
		vd.m_vertices = Stream(FLOAT_3D, sizeof(vertices[0]), false, (U8 *)vertices.data() + offsetof(Vertex, pos));
		vd.m_colors[0] = Stream(COL_UINT8_4D, sizeof(vertices[0]), false, (U8 *)vertices.data() + offsetof(Vertex, color));
		vd.m_indices = Stream(IDX_UINT16_1D, sizeof(indices[0]), false, (U8 *)indices.data());
		initOnce = false;
	}
	return &vd;
}

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
		case CREATE_WINDOW:  re.CreateWnd(msg->data); break;
		case CLOSE_WINDOW:   re.CloseWnd(msg->data); break;
		case ADD_MODEL:      re.AddModel(msg->data); break;
		case ADD_SHADER:     re.AddShader(msg->data); break;
		case RELOAD_SHADER:  re.ReloadShader(msg->data); break;

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