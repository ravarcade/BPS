// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"


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
		case ADD_MODEL:         re.AddModel(msg->data); break;
		case ADD_TEXTURE:       re.AddTexture(msg->data); break;
		case RELOAD_SHADER:     re.ReloadShader(msg->data); break;
		case GET_SHADER_PARAMS: re.GetShaderParams(msg->data); break;
		case GET_OBJECT_PARAMS: re.GetObjectParams(msg->data); break;
		case UPDATE_DRAW_COMMANDS: re.UpdateDrawCommands(msg->data); break;
		case SET_CAMERA:        re.SetCamera(msg->data); break;
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