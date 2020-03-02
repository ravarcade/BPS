#include "stdafx.h"

namespace BAMS {
namespace CORE {

// ============================================================================ ResBase ===

void ResBase::_CreateResourceImplementation()
{
	// create resource
	globalResourceManager->CreateResourceImplementation(this);
}

}; // CORE namespace
}; // BAMS namespace