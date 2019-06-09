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
	void AddMesh(CSTR meshResourceName, const float *m = nullptr);
	

private:
	ResBase *rb;

	struct MeshEntry {
		ResBase *mesh;
		float m[16];
		MProperties prop;
	};

	
	basic_array<MeshEntry> meshes;
	void _LoadXML();
	void _SaveXML();

};