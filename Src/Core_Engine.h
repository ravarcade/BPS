/**
 * DO NOT INCLUDE THIS FILE DIRECTLY.
 * USE:
 *
 * #include "Core.h"
 *
 */

class EngineModule : public IModule
{
public:
	void Update(float dt);
	void Initialize();
	void Finalize();
	void SendMessage(Message *msg);
	U32 GetModuleId() { return IModule::EngineModule; }

	~EngineModule();
};
