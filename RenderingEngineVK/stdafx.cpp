// stdafx.cpp : source file that includes just the standard includes
// RenderingEngineVK.pch will be the pre-compiled header
// stdafx.obj will contain the pre-compiled type information

#include "stdafx.h"

// TODO: reference any additional headers you need in STDAFX.H
// and not in this file

// add glfw and vulkan libs
// LIB_SUFFIX = "_64" for 64 bit build and "d" for debug build
#pragma comment(lib, "glfw\\lib\\glfw3" LIB_SUFFIX ".lib")
#pragma comment(lib, "vulkan-1.lib")