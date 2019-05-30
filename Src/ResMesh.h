/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResMesh : public ResoureImpl<ResMesh, RESID_MESH, false> 
{
	ResourceBase *rb;
	VertexDescription vd;
	U32 meshIdx;
	U32 meshHash;
	ResourceBase *meshSrc;
	bool isVertexDescriptionDataSet;
	void _LoadXML();
	void _SaveXML();

public:
	ResMesh(ResourceBase *res) : rb(res), meshSrc(nullptr), meshIdx(-1), meshHash(0), isVertexDescriptionDataSet(false) {
		if (res->XML->FirstChild())
			_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
	}
	~ResMesh() {}

	void Update(ResourceBase *res) {}
	void Release(ResourceBase *res) {}

	void SetVertexDescription(VertexDescription *pvd, U32 _meshHash, ResourceBase *_meshSrc, U32 _meshIdx);

	VertexDescription *GetVertexDescription(bool loadASAP = false);
	ResourceBase *GetMeshSrc() { return meshSrc; }
	U32 GetMeshIdx() { return meshIdx; }
	void SetMeshIdx(U32 idx) { meshIdx = idx; }
	U32 GetMeshHash() { return meshHash; }
};

class ResImportModel : public ResoureImpl<ResImportModel, RESID_IMPORTMODEL, false>
{
	ResourceBase *rb;
	SIZE_T resourceSize;
	time_t resourceTimestamp;

public:
	ResImportModel(ResourceBase *res) : rb(res), resourceSize(res->GetSize()), resourceTimestamp(res->GetTimestamp()) {
	}
	~ResImportModel() {}

	void Update(ResourceBase *res) {
		// validate update: is anything changed or we just loaded/used resource?
		if (res->GetSize() != resourceSize || res->GetTimestamp() != resourceTimestamp)
		{
			resourceSize = res->GetSize();
			resourceTimestamp = res->GetTimestamp();
			IEngine::PostMsg(IMPORTMODULE_UPDATE, IMPORT_MODULE, 0, rb);
		}
	}

	void Release(ResourceBase *res) {}

	static void IdentifyResourceType(ResourceBase *res);
};