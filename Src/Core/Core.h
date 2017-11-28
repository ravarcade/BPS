#pragma once

#define NAMESPACE_CORE_BEGIN namespace BAMS { namespace CORE {
#define NAMESPACE_CORE_END } }
#define NAMESPACE_CORE BAMS::CORE

// windows libs:
#include <Rpc.h>	// for UUID
#include <stdint.h> // for int64_t and few similiary types
#include <stdlib.h> // for malloc, mfree

// standard c++ libs:
#include <vector>
//#include <list>

NAMESPACE_CORE_BEGIN

#include "BasicTypes.h"
#include "MemoryAllocator.h"
#include "CoreTypes.h"
#include "Tools.h"
#include "ResourceManager.h"

NAMESPACE_CORE_END