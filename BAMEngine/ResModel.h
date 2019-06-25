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
	void AddMesh(ResBase *mesh, ResBase *shader, const float *m = nullptr);
	void GetMesh(U32 idx, const char ** mesh, const char **shader, const float **m, const Properties **prop);
	void GetMesh(U32 idx, ResBase **mesh, ResBase **shader, const float **m, const Properties **prop);

	U32 GetMeshCount() { return static_cast<U32>(meshes.size()); }

private:
	ResBase *rb;

	struct MeshEntry 
	{
 		MeshEntry() = default;
		MeshEntry(ResBase *_mesh, ResBase *_shader, const float *_M, const Properties &_prop) : mesh(_mesh), shader(_shader), prop(_prop) { memcpy_s(M, sizeof(M), _M, sizeof(M)); }
		MeshEntry(ResBase *_mesh, ResBase *_shader, const float *_M) : mesh(_mesh), shader(_shader) 
		{ 
			memcpy_s(M, sizeof(M), _M, sizeof(M));
			CResMesh me(mesh); 
			prop = *me.GetMeshProperties(true); 
			FindTexturesInProperties(reinterpret_cast<ResBase *>(me.GetMeshSrc())); 
		}
		~MeshEntry() = default;
		void FindTexturesInProperties(ResBase *meshSrc);

		ResBase *mesh;
		ResBase *shader;
		float M[16];
		sProperties prop;
		
	};
	

	array<MeshEntry> meshes;
	void _LoadXML();
	void _SaveXML();
};