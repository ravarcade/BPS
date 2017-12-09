
// standard windows headers:
#include <windows.h>


#ifdef BPS_BUILD_DLL
#define BAMS_EXPORT __declspec(dllexport)

#include "BAMEngineInternal.h"

#else

#pragma  comment(lib, "BAMEngine.lib")
#define BAMS_EXPORT __declspec(dllimport)

#include "BAMEngineImport.h"

#endif

//#include "ResourcesManifest.h"