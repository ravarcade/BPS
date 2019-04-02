#pragma once

using namespace Assimp;
using namespace BAMS::RENDERINENGINE;

class Import 
{

public:
	Import();

	void Load(const wchar_t *filename, const char *name = NULL, IOSystem *io = NULL, bool importMaterials = true);
	void Load(const uint8_t *pBuffer, size_t length, const char *name = NULL, Assimp::IOSystem *io = NULL, bool importMaterials = true);
//	RE::Bones *m_pBones;
//	vector<ImportedMaterial *> m_pMaterials;

//	Model * GetModel();

//	void DbgPrint();


private:
	void SetVD2(VertexDescription & vd, aiMesh * mesh);
	void SetVD(VertexDescription & vd, aiMesh * mesh);
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
};