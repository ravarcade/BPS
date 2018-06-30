/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core\Core.h"
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

#include "CoreType_array.h"
#include "CoreType_helpers.h"
#include "CoreType_strings.h"
#include "CoreType_hash.h"
#include "CoreType_queue.h"
#include "CoreType_hashtable.h"
#include "CoreType_list.h"
