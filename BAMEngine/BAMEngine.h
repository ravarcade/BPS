
// standard windows headers:




// windows libs:
#include <windows.h>
#include <Rpc.h>	// for UUID
#include <locale>   // for setlocale

// standard c++ libs:
#include <stdint.h> // for int64_t and few similiary types
#include <stdlib.h> // for malloc, mfree
#include <thread>
#include <chrono>
#include <mutex>
#include <utility>

// MACRO-s:
#ifdef NDEBUG
#define assert(condition) ((void)0)
#define TRACE(x) {}
#define TRACEW(x)
#else
#include <cassert>
#  ifdef _MSC_VER
#    include <windows.h>
#    include <sstream>
#    define TRACE(x)                           \
     do {  std::ostringstream s;  s << x;      \
           OutputDebugStringA(s.str().c_str()); \
        } while(0)
#    define TRACEW(x)                           \
     do {  std::wstringstream s;  s << x;      \
           OutputDebugStringW(s.str().c_str()); \
        } while(0)
#  else
#    include <iostream>
#    define TRACE(x)  std::cerr << x << std::flush
#    define TRACEW(x)  std::cerr << x << std::flush
#  endif
#endif

#define TRACEM(m) { if (false) {TRACE(#m << ":\n"); for (uint32_t y=0; y<4; ++y) { for (uint32_t x=0; x<4; ++x) {TRACE(m[x][y]); if (x<3) TRACE(", ");} TRACE("\n");}}}

template< class Type, ptrdiff_t n >
constexpr ptrdiff_t COUNT_OF(Type const (&)[n]) noexcept { return n; }

#ifdef BPS_BUILD_DLL
#define BAMS_EXPORT __declspec(dllexport)
#else
#define BAMS_EXPORT __declspec(dllimport)
#pragma  comment(lib, "BAMEngine.lib")
#endif

// Parts of BAMS:
#include "..\3rdParty\tinyxml2\tinyxml2.h"

namespace BAMS {
	
#include "Core_BasicTypes.h"
#include "Core_MemoryAllocator.h"
#include "Core_Types.h"
#include "Core_Message.h"
#include "Core_Tools.h"

#include "RenderingEngine_Config.h"
#include "RenderingEngine_VertexDescription.h"
#include "RenderingEngine_Image.h"

#include "DllInterface_IResourceManager.h"

};