#include "stdafx.h"

NAMESPACE_CORE_BEGIN

struct ModuleRegistrationBase : IModule
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
	void SendMessage(Message *msg) { _Module.SendMessage(msg); }
	U32 GetModuleId() { return _Module.GetModuleId(); }
	IModule *GetModule() { return &_Module; }
};

ModuleRegistrationBase *ModuleRegistrationBase::first = nullptr;

#define MODULE(x) ModuleRegistration< x > x##_registered

MODULE(ResourceManagerModule);
MODULE(EngineModule);


void IEngine::Update(float dT)
{
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
}

IModule *IEngine::GetModule(U32 moduleId)
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		if (f->GetModuleId() == moduleId)
			return f->GetModule();

	return nullptr;
}

void IEngine::SendMessage(Message *msg)
{
	for (auto f = ModuleRegistrationBase::first; f; f = f->next)
		f->SendMessage(msg);
}

NAMESPACE_CORE_END

