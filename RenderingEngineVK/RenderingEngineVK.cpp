// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "BAMEngine.h"

using namespace BAMS;
using namespace BAMS::RENDERINENGINE;

VertexDescription *GetDemoCube()
{
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
		vd.m_vertices = Stream(VAT_FLOAT_3D, sizeof(vertices[0]), false, (U8 *)vertices.data() + offsetof(Vertex, pos));
		vd.m_colors[0] = Stream(VAT_COL_UINT8_4D, sizeof(vertices[0]), false, (U8 *)vertices.data() + offsetof(Vertex, color));
		vd.m_indices = Stream(VAT_IDX_UINT16_1D, sizeof(indices[0]), false, (U8 *)indices.data());
		initOnce = false;
	}
	return &vd;
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
		case CREATE_WINDOW:     re.CreateWnd(msg->data); break;
		case CLOSE_WINDOW:      re.CloseWnd(msg->data); break;
		case ADD_SHADER:        re.AddShader(msg->data); break;
		case ADD_MESH:          re.AddMesh(msg->data); break;
		case RELOAD_SHADER:     re.ReloadShader(msg->data); break;
		case GET_SHADER_PARAMS: re.GetShaderParams(msg->data); break;
		case GET_OBJECT_PARAMS: re.GetObjectParams(msg->data); break;
		}
	}

	void Destroy() {}
};


RenderingModule RM;

extern "C" {
	REVK_EXPORT void RegisterModule()
	{
		printf("HELLO REVK\n");
		RM.Register();
		
	}
}