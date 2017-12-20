#pragma once

#define NAMESPACE_CORE_BEGIN namespace BAMS { namespace CORE {
#define NAMESPACE_CORE_END } }
#define NAMESPACE_CORE BAMS::CORE

// windows libs:
#include <Rpc.h>	// for UUID
#include <locale>   // for setlocale

// standard c++ libs:
#include <stdint.h> // for int64_t and few similiary types
#include <stdlib.h> // for malloc, mfree
#include <vector>
//#include <list>

#ifdef NDEBUG
#define assert(condition) ((void)0) 
#else
#include <cassert>
#endif

NAMESPACE_CORE_BEGIN

#include "BasicTypes.h"
#include "MemoryAllocator.h"
#include "CoreTypes.h"
#include "Tools.h"
#include "ResourceManager.h"
#include "RawData.h"

NAMESPACE_CORE_END