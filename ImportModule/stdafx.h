// stdafx.h : include file for standard system include files,
// or project specific include files that are used frequently, but
// are changed infrequently
//

#pragma once

#include "targetver.h"

#define WIN32_LEAN_AND_MEAN             // Exclude rarely-used stuff from Windows headers
// Windows Header Files
#include <windows.h>



// reference additional headers your program requires here
#include "BAMEngine.h"
#pragma  comment(lib, "BAMEngine.lib")

// glm
#include <glm\glm.hpp>
#include <glm\gtc\quaternion.hpp>
#include <glm\gtc\constants.hpp>
#include <glm\gtc\matrix_transform.hpp>
#include <glm\gtc\matrix_inverse.hpp>

// assimp
#include <assimp/cimport.h>
#include <assimp/types.h>
#include <assimp/Importer.hpp> 
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <assimp/DefaultLogger.hpp> 
#include <assimp/IOSystem.hpp>
#include <assimp/IOStream.hpp>

#include "string.h"

#define IM_BUILD_DLL
#include "AssImp_Loader.h"
#include "image.h"
#include "ImportModule.h"