#pragma once

class CStringStorage
{
public:
	CStringStorage(size_t step = 4096) : _step(step), _first(nullptr) {}
	~CStringStorage() { clear(); }

	const char *add(const char *txt, size_t len = 0)
	{
		if (!len) len = strlen(txt);
		++len;

		if (!_first || (_first->used + len) > _step)
		{
			chunk *last = _first;
			_first = reinterpret_cast<chunk *>(new char[_step + sizeof(chunk)]);
			_first->used = 0;
			_first->next = last;
		}

		char *dst = reinterpret_cast<char *>(_first + 1) + _first->used;
		memcpy_s(dst, len, txt, len);
		_first->used += len;
		return dst;
	}

	void clear()
	{
		for (chunk *f = _first; f;)
		{
			auto d = f;
			f = f->next;
			delete d;
		}
		_first = nullptr;
	}

private:
	size_t _step;

	struct chunk {
		chunk *next;
		size_t used;
	};

	chunk *_first;
};

class AssImp_Loader 
{

public:
	struct ImportedMesh {
		BAMS::VertexDescription vd;
		uint32_t hash;
		bool match;
		BAMS::sProperties prop;
	};

	AssImp_Loader();
	~AssImp_Loader();

	void PreLoad(const wchar_t *filename, const char *name = NULL, Assimp::IOSystem *io = NULL, bool importMaterials = true);
	void PreLoad(const uint8_t *pBuffer, size_t length, const char *name = NULL, Assimp::IOSystem *io = NULL, bool importMaterials = true);
//	RE::Bones *m_pBones;
//	vector<ImportedMaterial *> m_pMaterials;

//	Model * GetModel();

//	void DbgPrint();

	template<typename T>
	bool ForEachMesh(T f)
	{
		for (auto &m : _meshes)
		{
			if (f(m))
				return true;
		}

		return false;
	}

	ImportedMesh *Mesh(uint32_t idx) { return idx < _meshes.size() ? &_meshes[idx] : nullptr; }
	
	bool IsLoaded() { return _isLoaded; }
	void OnFinalize() { Clear(); }

	void Clear() {
		_aii.FreeScene();
		u16_indicesData.clear();
		u32_indicesData.clear();
		_meshes.clear();
	}

private:
	BAMS::U32 _JSHash(BAMS::VertexDescription &vd, BAMS::U32 hash = 0);
	void AddMesh(aiMesh * mesh);
	void _AddMaterial(ImportedMesh * m, aiMesh * aiMesh);

	void AddMesh_broken(aiMesh * mesh);
	void LoadBones();
	void LoadMeshes();
	void LoadAnims();
	void FillBoneTree(const aiNode* pNode, int parentNodeID);
	void ProcessScene();
	void _OptimizeMeshStorage();

	bool   _isLoaded;
	bool   _importMaterials;
	uint8_t *_meshBinData;
	size_t _meshBinDataSize;

	Assimp::Importer _aii;
	glm::vec3 minTex, maxTex;
	std::string _fileName;		// file name is UTF8 encoded

	const aiScene* _pScene;

	//	Model *m_pModel;
	//std::string _modelName;

	std::vector<uint16_t> u16_indicesData;
	std::vector<uint32_t> u32_indicesData;

	std::vector<ImportedMesh> _meshes;
	CStringStorage _cstringStorage;
	//void _SafeMProperties(BAMS::MProperties &dst);
};