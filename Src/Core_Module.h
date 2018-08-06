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

};
