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

#define GLFW_INCLUDE_VULKAN
#include "glfw\glfw3.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include "glm\vec4.hpp"
#include "glm\mat4x4.hpp"

