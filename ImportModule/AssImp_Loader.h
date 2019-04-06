#pragma once

using namespace Assimp;
using namespace BAMS::RENDERINENGINE;

class AssImp_Loader 
{

public:
	AssImp_Loader();

	void PreLoad(const wchar_t *filename, const char *name = NULL, IOSystem *io = NULL, bool importMaterials = true);
	void PreLoad(const uint8_t *pBuffer, size_t length, const char *name = NULL, Assimp::IOSystem *io = NULL, bool importMaterials = true);
//	RE::Bones *m_pBones;
//	vector<ImportedMaterial *> m_pMaterials;

//	Model * GetModel();

//	void DbgPrint();

	template<typename T>
	void ForEachMesh(T f)
	{
		for (auto &vd : _meshes)
			f(vd);
	}

private:
	void AddMesh(aiMesh * mesh);
	void AddMesh_broken(aiMesh * mesh);
	void LoadBones();
	void LoadMeshes();
	void LoadAnims();
	void FillBoneTree(const aiNode* pNode, int parentNodeID);
	void ProcessScene();

	Importer _aii;
	glm::vec3 minTex, maxTex;
	std::string _fileName;
	bool _importMaterials;

	const aiScene* _pScene;

	//	Model *m_pModel;
	std::string _modelName;

	std::vector<uint16_t> u16_indicesData;
	std::vector<uint32_t> u32_indicesData;

	std::vector<VertexDescription> _meshes;
};