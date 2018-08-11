#include "stdafx.h"


NAMESPACE_CORE_BEGIN

// ============================================================================

struct ClassRegistrationBase
{
	virtual void CallInitialize() = 0;
	virtual void CallFinalize() = 0;
	ClassRegistrationBase *next;
	static ClassRegistrationBase *first;

	ClassRegistrationBase() { next = first; first = this; }

};

template <typename T>
struct ClassRegistration : ClassRegistrationBase
{
	void CallInitialize() { T::Initialize(); }
	void CallFinalize() { T::Finalize(); }
};

template <typename T>
struct ModuleRegistration : ClassRegistrationBase
{
	T _Module;

	void CallInitialize() { _Module.Initialize(); }
	void CallFinalize() { _Module.Finalize(); }
};

ClassRegistrationBase *ClassRegistrationBase::first = nullptr;

#define REGISTER(x) ClassRegistration< x > x##_registered

// ============================================================================

REGISTER(STR);
REGISTER(WSTR);
REGISTER(PathSTR);
REGISTER(ShortSTR);

// ============================================================================

void RegisteredClasses::Initialize()
{
	for (auto f = ClassRegistrationBase::first; f; f = f->next)
		f->CallInitialize();
}

void RegisteredClasses::Finalize()
{
	for (auto f = ClassRegistrationBase::first; f; f = f->next)
		f->CallFinalize();
}


NAMESPACE_CORE_END