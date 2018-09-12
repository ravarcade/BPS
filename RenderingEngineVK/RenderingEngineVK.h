#pragma once

#ifdef REVK_BUILD_DLL
#define REVK_EXPORT __declspec(dllexport)
#else
#define REVK_EXPORT __declspec(dllimport)
#endif
