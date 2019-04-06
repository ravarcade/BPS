/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResMesh : public ResoureImpl<ResMesh, RESID_MESH, Allocators::default, false> 
{
	ResourceBase *rb;
	BAMS::RENDERINENGINE::VertexDescription vd;
	U32 meshIdx;
	ResourceBase *meshSrc;

	void _LoadXML();
	void _SaveXML();

public:
	ResMesh(ResourceBase *res) : rb(res), meshSrc(nullptr), meshIdx(-1) {
		TRACE("[ResMesh]\n");
		_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
	}
	~ResMesh() {}

	void Update(ResourceBase *res) {}
	void Release(ResourceBase *res) {}

	bool IsSame(BAMS::RENDERINENGINE::VertexDescription &_vd, ResourceBase *_meshSrc, U32 _meshIdx) { return _meshSrc == meshSrc && _meshIdx == meshIdx && _vd == vd; }
	void SetVertexDescription(BAMS::RENDERINENGINE::VertexDescription &_vd, ResourceBase *_meshSrc, U32 _meshIdx) { vd = _vd; meshSrc = _meshSrc; meshIdx = _meshIdx; _SaveXML(); }
	static STR BuildXML(BAMS::RENDERINENGINE::VertexDescription &_vd, ResourceBase *_meshSrc, U32 _meshIdx);
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