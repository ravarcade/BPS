/**
* DO NOT INCLUDE THIS FILE DIRECTLY.
* USE:
*
* #include "Core.h"
*
*/

class ResModel : public ResImp<ResModel, RESID_MODEL, false>
{
public:
	ResModel(ResBase *res) : rb(res) {
		if (rb->XML->FirstChild())
			_LoadXML(); // XML is readed from manifest, so no need to load any file from disk
	}
	~ResModel() {}
	void Update(ResBase *res) {}
	void Release(ResBase *res) {}

	// ResModel methods
	void AddMesh(CSTR meshResourceName, CSTR shaderProgramName, const float *m = nullptr);
	void GetMesh(U32 idx, const char ** mesh, const char **shader, const float **m, MProperties **prop);
	U32 GetMeshCount() { return static_cast<U32>(meshes.size()); }

private:
	ResBase *rb;

	struct MeshEntry {
		ResBase *mesh;
		ResBase *shader;
		float m[16];
		MProperties prop;
	};

	
	basic_array<MeshEntry> meshes;
	void _LoadXML();
	void _SaveXML();

};