/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class IModule
{
public:
	virtual ~IModule() {}

	virtual void Update(float dt) = 0;
	virtual void Initialize(void) = 0;
	virtual void Finalize(void) = 0;

	// This recieves any messages sent to the core engine in Engine.cpp
	virtual void SendMsg(Message *msg) = 0;

	virtual U32 GetModuleId(void) = 0;

	enum {
		EngineModule = 0x100,
		ResourceManagerModule = 0x101
		// 0x200-0x2ff - rendering module
	};
};
/*
template<U32 id>
class IModuleId : IModule
{
protected:
	static const U32 __moduleId = id;
public:
	U32 GetModuleId() { return __moduleId; }
};
*/

struct IEngine
{
	typedef void ExternalModule_Initialize(const void *moduleData);
	typedef void ExternalModule_Finalize(const void *moduleData);
	typedef void ExternalModule_Update(float dt, const void *moduleData);
	typedef void ExternalModule_SendMsg(Message *msg, const void *moduleData);
	typedef void ExternalModule_Destroy(const void *moduleData);
	class ExternalModule;

	static void Update(float dt);
	static void Initialize();
	static void Finalize();
	static IModule *GetModule(U32 moduleId);

	static void SendMsg(Message *msg);
	static void PostMsg(Message *msg, time::duration delay = 0ms);

	static void RegisterExternalModule(
		uint32_t moduleId,
		ExternalModule_Initialize *moduleInitialize,
		ExternalModule_Finalize *moduleFinalize,
		ExternalModule_Update *moduleUpdate,
		ExternalModule_SendMsg *moduleSendMessage,
		ExternalModule_Destroy *moduleDestroy,
		const void *moduleData);
};





