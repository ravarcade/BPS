/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResMesh : public ResoureImpl<ResMesh, RESID_MESH, Allocators::default>
{
	ResourceBase *rb;
	BAMS::RENDERINENGINE::VertexDescription vd;
	WSTR dataFilename;
	U32 meshIdx;

public:
	ResMesh(ResourceBase *res) : rb(res), meshIdx(-1) {
		TRACE("[ResMesh]\n");
	}
	~ResMesh() {}

	void Update(ResourceBase *res)
	{
	}

	void Release(ResourceBase *res)
	{
		res->ResourceLoad(nullptr);
	}
};

class ResImportModel : public ResoureImpl<ResImportModel, RESID_IMPORTMODEL, Allocators::default, false>
{
	ResourceBase *rb;
	SIZE_T resourceSize;
	time_t resourceTimestamp;
public:
	ResImportModel(ResourceBase *res) : rb(res), resourceSize(res->GetSize()), resourceTimestamp(res->GetTimestamp()) {
		TRACE("[ResImportModel]\n");
	}
	~ResImportModel() {}

	void Update(ResourceBase *res) { 
		// validate update: is anything changed or we just loaded/used resource?
		if (res->GetSize() != resourceSize || res->GetTimestamp() != resourceTimestamp)
		{
			resourceSize = res->GetSize();
			resourceTimestamp = res->GetTimestamp();
			IEngine::PostMsg(IMPORTMODEL_UPDATE, IMPORT_MODULE, 0, rb);
		}
	}

	void Release(ResourceBase *res) {}

	static void IdentifyResourceType(ResourceBase *res);
};