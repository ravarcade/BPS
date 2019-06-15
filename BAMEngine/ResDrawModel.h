/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResDrawModel : public ResImp<ResDrawModel, RESID_DRAWMODEL, false>
{
public:
	ResDrawModel(ResBase *res) : rb(res) {
		if (rb->XML->FirstChild())
			_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
	}
	~ResDrawModel() {}
	void Update(ResBase *res) {}
	void Release(ResBase *res) {}

	// ResDrawModel methods
	void SetModel(CSTR model) {};
	void GetModel() {};
	void SetMatrix(const F32 *m) {};
	MProperties* GetMeshProperties(U32 idx) { return nullptr; }
	U32 GetMeshCount() { return 0;  }

private:
	ResBase *rb;
/*
	struct MeshEntry {
		ResBase *mesh;
		ResBase *shader;
		float m[16];
		MProperties prop;
	};


	basic_array<MeshEntry> meshes;
*/
	void _LoadXML();
	void _SaveXML();

};
