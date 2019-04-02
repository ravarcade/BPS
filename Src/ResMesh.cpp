#include "stdafx.h"

NAMESPACE_CORE_BEGIN

void ResImportModel::IdentifyResourceType(ResourceBase * res)
{
	if (res->Type != RESID_UNKNOWN)
		return;
	PIDETIFY_RESOURCE ir = {
		res->Path.c_str(),
		&res->Type,
		res};
	IEngine::SendMsg(IDENTIFY_RESOURCE, IMPORT_MODULE, 0, &ir);
}

NAMESPACE_CORE_END