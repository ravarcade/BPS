/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

struct Message
{
	U32 source;
	U32 targetModule;
	U32 id;
	void *data;
};