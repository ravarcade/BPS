#include "stdafx.h"
#include "..\..\BAMEngine\stdafx.h"

NAMESPACE_CORE_BEGIN

size_t Allocators::Default::CurrentAllocatedMemory = 0;
size_t Allocators::Default::MaxAllocatedMemory = 0;
size_t Allocators::Default::TotalAllocateCommands = 0;

NAMESPACE_CORE_END