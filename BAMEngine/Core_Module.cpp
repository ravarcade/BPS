#include "stdafx.h"

namespace BAMS {
namespace CORE {

struct ModuleRegistrationBase :IModule
{
	ModuleRegistrationBase *next;
	static ModuleRegistrationBase *first;

	ModuleRegistrationBase() { next = first; first = this; }
	virtual IModule *GetModule() = 0;
};


template <typename T>
struct ModuleRegistration : ModuleRegistrationBase
{
	T _Module;
		
	void Update(float dt) { _Module.Update(dt); }
	void Initialize() { _Module.Initialize(); }
	void Finalize() { _Module.Finalize(); }
	void SendMsg(Message *msg) { _Module.SendMsg(msg); }
	U32 GetModuleId() { return _Module.GetModuleId(); }
	IModule *GetModule() { return &_Module; }
};

ModuleRegistrationBase *ModuleRegistrationBase::first = nullptr;

#define MODULE(x) ModuleRegistration< x > x##_registered

MODULE(ResourceManagerModule);
MODULE(EngineModule);

class IEngine::ExternalModule : ModuleRegistrationBase
{
private:
	U32 _moduleId;
	ExternalModule_Initialize *_Initialize;
	ExternalModule_Finalize *_Finalize;
	ExternalModule_Update *_Update;
	ExternalModule_SendMsg *_SendMsg;
	ExternalModule_Destroy *_Destroy;
	const void *_moduleData;

public:
	ExternalModule() :
		_moduleId(0),
		_Initialize(nullptr),
		_Finalize(nullptr),
		_Update(nullptr),
		_SendMsg(nullptr),
		_Destroy(nullptr),
		_moduleData(nullptr),
		ModuleRegistrationBase()
	{}

	ExternalModule(uint32_t moduleId,
		ExternalModule_Initialize *moduleInitialize,
		ExternalModule_Finalize *moduleFinalize,
		ExternalModule_Update *moduleUpdate,
		ExternalModule_SendMsg *moduleSendMsg,
		ExternalModule_Destroy *moduleDestroy,
		const void *moduleData)
		:
		_moduleId(moduleId),
		_Initialize(moduleInitialize),
		_Finalize(moduleFinalize),
		_Update(moduleUpdate),
		_SendMsg(moduleSendMsg),
		_Destroy(moduleDestroy),
		_moduleData(moduleData),
		ModuleRegistrationBase()
	{}

	~ExternalModule() { _Destroy(_moduleData); }
	void Initialize(void) { _Initialize(_moduleData); }
	void Finalize(void) { _Finalize(_moduleData); }
	void Update(float dt) { _Update(dt, _moduleData); }
	void SendMsg(Message *msg) { _SendMsg(msg, _moduleData); }
	U32 GetModuleId(void) { return _moduleId; }
	IModule *GetModule() { return this; }
};


basic_array<IEngine::ExternalModule> externalModules;

struct QMessage {
	Message msg;
	time triggerTime;
	IMemoryAllocator *alloc;

	QMessage() = delete;
	QMessage(const QMessage &) = delete;
	QMessage(QMessage &&) = delete;
	QMessage &operator == (const QMessage &) = delete;
	QMessage &operator == (QMessage &&) = delete;

	QMessage(IMemoryAllocator *_alloc, Message *m, time::duration delay) :
		msg(*m), 
		triggerTime(clock::now() + delay),
		alloc(_alloc)
	{
		if (msg.dataLen) {
			auto buf = alloc->allocate(msg.dataLen);
			memcpy_s(buf, msg.dataLen, msg.data, msg.dataLen);
			msg.data = buf;
		}
	}
	~QMessage() 
	{
		if (msg.dataLen) {
			alloc->deallocate(msg.data);
		}

	}
};

queue<QMessage> messageQueue;

void IEngine::Update(float dT)
{
	auto now = clock::now();

	messageQueue.filter([&](QMessage *m) { // selector
		return now > m->triggerTime;
	}, [&](QMessage *m) { // process
		SendMsg(&m->msg);
	});
	

	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		f->Update(dT);
}

void IEngine::Initialize()
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		f->Initialize();
}

void IEngine::Finalize()
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		f->Finalize();
	externalModules.reset();
	messageQueue.reset();
	ModuleRegistrationBase::first = nullptr;
}

IModule *IEngine::GetModule(U32 moduleId)
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		if (f->GetModuleId() == moduleId)
			return f->GetModule();

	return nullptr;
}

void IEngine::SendMsg(Message *msg)
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		if (msg->destination == 0 || msg->destination == f->GetModuleId())
			f->SendMsg(msg);
}

void IEngine::SendMsg(uint32_t msgId, uint32_t msgDst, uint32_t msgSrc, void * data, uint32_t dataLen)
{
	Message msg = {
		msgId,
		msgDst,
		msgSrc,
		data,
		dataLen
	};
	SendMsg(&msg);
}

void IEngine::PostMsg(Message *msg, time::duration delay)
{
	messageQueue.push_back(msg, delay);
}

void IEngine::PostMsg(uint32_t msgId, uint32_t msgDst, uint32_t msgSrc, void * data, uint32_t dataLen, time::duration delay)
{
	Message msg = {
		msgId,
		msgDst,
		msgSrc,
		data,
		dataLen
	};
	PostMsg(&msg, delay);
}

void IEngine::RegisterExternalModule(
	uint32_t moduleId,
	ExternalModule_Initialize *moduleInitialize,
	ExternalModule_Finalize *moduleFinalize,
	ExternalModule_Update *moduleUpdate,
	ExternalModule_SendMsg *moduleSendMsg,
	ExternalModule_Destroy *moduleDestroy,
	const void *moduleData)
{
	auto em = externalModules.add_empty();
	new (em) ExternalModule(moduleId, moduleInitialize, moduleFinalize, moduleUpdate, moduleSendMsg, moduleDestroy, moduleData);
	em->Initialize();
}

}; // CORE namespace
}; // BAMS namespace

