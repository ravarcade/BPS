// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "BAMEngine.h"

using namespace BAMS;

class RenderingModule : public IExternalModule<RenderingModule, RENDERING_ENGINE>
{
public:
	void Initialize() { printf("VK - Initialize\n"); }
	void Finalize() { printf("VK - Finalize\n"); }
	void Update(float dt) { printf("VK - Update(%f)\n", dt); }
	void SendMsg(Message *msg) 
	{ 
		switch (msg->id)
		{
		case CREATE_3D_WINDOW:
			printf("VK - SendMessage(%d) : CREATE_3D_WINDOW\n", msg->id);
			break;
		}
	}
	void Destroy() { printf("VK - Destroy\n"); }
};

RenderingModule RM;

extern "C" {
	REVK_EXPORT void RenderingEngine_RegisterModule()
	{
		printf("HELLO REVK\n");
		RM.Register();
		
	}

}