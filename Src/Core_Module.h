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
	virtual void SendMessage(Message *msg) = 0;

	virtual U32 GetModuleId(void) = 0;

	enum {
		EngineModule = 0x100,
		ResourceManagerModule = 0x101
	};
};

template<U32 id>
class IModuleId : IModule
{
protected:
	static const U32 __moduleId = id;
public:
	U32 GetModuleId() { return __moduleId; }
};

struct IEngine
{
	static void Update(float dt);
	static void Initialize();
	static void Finalize();
	static IModule *GetModule(U32 moduleId);

	static void SendMessage(Message *msg);
};


