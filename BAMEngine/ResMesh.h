/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResMesh : public ResImp<ResMesh, RESID_MESH, false> 
{
public:
	ResMesh(ResBase *res) : rb(res), meshSrc(nullptr), meshIdx(-1), meshHash(0), isMeshLoaded(false) {
		_LoadXML(); 
	}
	~ResMesh() {
	}

	void Update(ResBase *res) {}
	void Release(ResBase *res) {}

	// ResMesh methods
	void SetVertexDescription(VertexDescription *pvd, U32 _meshHash, ResBase *_meshSrc, U32 _meshIdx, const Properties *_meshProperties);
	const VertexDescription *GetVertexDescription(bool loadASAP = false);
	const Properties *GetMeshProperties(bool loadASAP = false);
	ResBase *GetMeshSrc() { return meshSrc; }
	U32 GetMeshIdx() { return meshIdx; }
	void SetMeshIdx(U32 idx) { meshIdx = idx; }
	U32 GetMeshHash() { return meshHash; }

private:
	ResBase *rb;
	VertexDescription vd;
	sProperties meshProperties;
	U32 meshIdx;
	U32 meshHash;
	ResBase *meshSrc;
	bool isMeshLoaded;
	void _LoadXML();
	void _SaveXML();

};

class ResImportModel : public ResImp<ResImportModel, RESID_IMPORTMODEL, false>
{
	ResBase *rb;
	SIZE_T resourceSize;
	time_t resourceTimestamp;

public:
	ResImportModel(ResBase *res) : rb(res), resourceSize(res->GetSize()), resourceTimestamp(res->GetTimestamp()) {
	}
	~ResImportModel() {}

	void Update(ResBase *res) {
		// validate update: is anything changed or we just loaded/used resource?
		if (res->GetSize() != resourceSize || res->GetTimestamp() != resourceTimestamp)
		{
			resourceSize = res->GetSize();
			resourceTimestamp = res->GetTimestamp();
			IEngine::PostMsg(IMPORTMODULE_UPDATE, IMPORT_MODULE, 0, rb);
		}
	}

	void Release(ResBase *res) {}

	static void IdentifyResourceType(ResBase *res);
};