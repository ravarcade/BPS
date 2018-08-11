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
#include <thread>
#include <chrono>
#include <mutex>

#ifdef NDEBUG
#define assert(condition) ((void)0)
#define TRACE(x)
#else
#include <cassert>
#  ifdef _MSC_VER
#    include <windows.h>
#    include <sstream>
#    define TRACE(x)                           \
     do {  std::ostringstream s;  s << x;      \
           OutputDebugStringA(s.str().c_str()); \
        } while(0)
#  else
#    include <iostream>
#    define TRACE(x)  std::cerr << x << std::flush
#  endif
#endif

#include "Core_BasicTypes.h"

NAMESPACE_CORE_BEGIN

#include "Core_MemoryAllocator.h"
#include "Core_Types.h"
#include "Core_Message.h"
#include "Core_Module.h"
#include "Core_Tools.h"
#include "Core_DirectoryChangeNotifier.h"
#include "Core_ResourceManager.h"
#include "Core_RawData.h"
#include "Core_Engine.h"

NAMESPACE_CORE_END