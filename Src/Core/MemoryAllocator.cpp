#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

NAMESPACE_CORE_BEGIN

size_t Allocators::Standard::CurrentAllocatedMemory = 0;
size_t Allocators::Standard::MaxAllocatedMemory = 0;
size_t Allocators::Standard::TotalAllocateCommands = 0;

size_t Allocators::Debug::CurrentAllocatedMemory = 0;
size_t Allocators::Debug::MaxAllocatedMemory = 0;
size_t Allocators::Debug::TotalAllocateCommands = 0;
U32 Allocators::Debug::Counter = 0;
Allocators::Debug::ExtraMemoryBlockInfo *Allocators::Debug::Last = nullptr;

NAMESPACE_CORE_END