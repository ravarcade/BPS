/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

/// <summary>
/// We may need to call Initialize & Finalize befor and after we start using some classes.
/// This is helper class for this purpose. Also it is used to "register classes".
/// </summary>
struct RegisteredClasses
{
	static void Initialize();
	static void Finalize();
};

// ============================================================================
using namespace std::chrono_literals; // we want to use literals, like 50ms or 1h
typedef std::chrono::steady_clock clock;
typedef std::chrono::steady_clock::time_point time;

#include "Core_Array.h"
#include "Core_Helpers.h"
#include "Core_Strings.h"
#include "Core_Hash.h"
#include "Core_Queue.h"
#include "Core_Hashtable.h"
#include "Core_List.h"
#include "Core_Properties.h"
