#pragma once

#ifdef REVK_BUILD_DLL
#define REVK_EXPORT __declspec(dllexport)
#else
#define REVK_EXPORT __declspec(dllimport)
#endif

#ifdef _WIN64
# ifdef NDEBUG
#  define LIB_SUFFIX "_x64"
# else
#  define LIB_SUFFIX "_x64d"
# endif
#else
# ifdef NDEBUG
#  define LIB_SUFFIX ""
# else
#  define LIB_SUFFIX "d"
# endif
#endif

#include <intrin.h> // _BitScanForward

#define GLFW_INCLUDE_VULKAN
#include <glfw\glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm\vec4.hpp>
#include <glm\mat4x4.hpp>

enum {
	MAINWND = 0,
	BACKBOXWND = 1,
	DMDWND = 2,
	MAX_WINDOWS
};

const bool ForceHostVisibleUBOS = true;

using namespace BAMS;

#include "Utils.h"
#include "ShadersReflections.h"
#include "ShaderProgram.h"
#include "Texture2d.h"
#include "iglfw.h"
#include "OutputWindow.h"
#include "ire.h"

