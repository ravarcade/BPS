/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

struct Message
{
	U32 id;
	U32 destination;
	U32 source;
	const void *data;
	U32 dataLen;
};