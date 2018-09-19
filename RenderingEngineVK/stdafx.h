// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files:
#include <windows.h>
#include <iostream>
#include <stdexcept>
#include <vector>
#include <set>
#include <array>
#include <algorithm>

// TODO: reference additional headers your program requires here
#define REVK_BUILD_DLL
#include "RenderingEngineVK.h"


#include "BAMEngine.h"
#pragma  comment(lib, "BAMEngine.lib")

