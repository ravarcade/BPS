#pragma once

#ifdef IM_BUILD_DLL
#define IM_EXPORT __declspec(dllexport)
#else
#define IM_EXPORT __declspec(dllimport)
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