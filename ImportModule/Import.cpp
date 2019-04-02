#include "stdafx.h"

Import::Import() :
	minTex(0), maxTex(0)
{
}

void Import::Load(const wchar_t * filename, const char * name, Assimp::IOSystem * io, bool importMaterials)
{
	unsigned int flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_RemoveRedundantMaterials | aiProcess_JoinIdenticalVertices;

	BAMS::CORE::STR cvt;
	cvt.UTF8(const_cast<wchar_t *>(filename));
	_fileName = cvt.c_str();
	_importMaterials = importMaterials;


	_aii.SetIOHandler(io);
	_pScene = _aii.ReadFile(cvt.c_str(), flags);
	if (_pScene)
	{
		ProcessScene();
		_aii.FreeScene();
	}
}

void Import::Load(const uint8_t * pBuffer, size_t length, const char * name, Assimp::IOSystem * io, bool importMaterials)
{
	unsigned int flags = aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_FlipUVs | aiProcess_RemoveRedundantMaterials | aiProcess_JoinIdenticalVertices;

	_importMaterials = importMaterials;

	_aii.SetIOHandler(io);
	_pScene = _aii.ReadFileFromMemory(pBuffer, length, flags, "ms3d");
	if (_pScene)
	{
		ProcessScene();
		_aii.FreeScene();
	}
}

void Import::LoadBones()
{
}


void Import::SetVD2(VertexDescription &vd, aiMesh *mesh)
{
	vd.m_numVertices = mesh->mNumVertices;
	vd.m_vertices = Stream(&mesh->mVertices[0][0]);

	if (mesh->HasNormals())
		vd.m_normals = Stream(&mesh->mNormals[0][0]);

	if (mesh->HasTangentsAndBitangents()) {
		vd.m_tangents = Stream(&mesh->mTangents[0][0]);
		vd.m_bitangents = Stream(&mesh->mBitangents[0][0]);
	}

	for (int i = 0; i < COUNT_OF(vd.m_colors); ++i)
	{
		if (mesh->HasVertexColors(i))
		{
			bool normalized = true;
			for (unsigned int j = 0; j < mesh->mNumVertices && normalized; ++j)
			{
				for (unsigned int k = 0; k < 4; ++k)
				{
					if (mesh->mColors[i][j][k] < 0.0 || mesh->mColors[i][j][k] > 1.0)
					{
						normalized = false;
						break;
					}
				}
			}
			vd.m_colors[i] = Stream(&mesh->mColors[i][0][0], 4);
			vd.m_colors[i].m_normalized = normalized;
		}
	}

	for (int i = 0; i < COUNT_OF(vd.m_textureCoords); ++i)
	{
		if (mesh->HasTextureCoords(i))
		{
			bool normalized = true;
			for (unsigned int j = 0; j < mesh->mNumVertices && normalized; ++j)
			{
				for (unsigned int k = 0; k < mesh->mNumUVComponents[i]; ++k)
				{
					if (mesh->mTextureCoords[i][j][k] < 0.0 || mesh->mTextureCoords[i][j][k] > 1.0)
					{
						normalized = false;
						break;
					}
				}
			}
			vd.m_textureCoords[i] = Stream(&mesh->mTextureCoords[i][0][0], mesh->mNumUVComponents[i], sizeof(mesh->mTextureCoords[i][0][0]) * 3);
			vd.m_textureCoords[i].m_normalized = normalized;
		}
	}

	if (mesh->HasFaces())
	{
		uint32_t faces = 0;
		bool uint16IsFine = true;
		const int MAX_UINT16 = 0xffff;

		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			if (mesh->mFaces[i].mNumIndices == 3)
			{ // only triangles
				for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
				{
					if (mesh->mFaces[i].mIndices[j] > MAX_UINT16)
					{
						uint16IsFine = false;
					}
				}
				++faces;
			}
		}

		if (faces)
		{
			vd.m_numIndices = faces * 3;
			uint16IsFine = false;

			if (uint16IsFine)
			{
				u16_indicesData.resize(faces * 3);
				vd.m_indices = Stream(&u16_indicesData[0], 0); // it is index, so len = 0
				UINT16 *p = (UINT16 *)vd.m_indices.m_data;
				for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
				{
					if (mesh->mFaces[i].mNumIndices != 3)
						continue;
					*p++ = (UINT16)mesh->mFaces[i].mIndices[0];
					*p++ = (UINT16)mesh->mFaces[i].mIndices[1];
					*p++ = (UINT16)mesh->mFaces[i].mIndices[2];
				}
			}
			else {
				u32_indicesData.resize(faces * 3);
				vd.m_indices = Stream(&u32_indicesData[0], 0); // it is index, so len = 0
				UINT32 *p = (UINT32 *)vd.m_indices.m_data;
				for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
				{
					if (mesh->mFaces[i].mNumIndices != 3)
						continue;
					*p++ = mesh->mFaces[i].mIndices[0];
					*p++ = mesh->mFaces[i].mIndices[1];
					*p++ = mesh->mFaces[i].mIndices[2];
				}
			}
		} // if (faces) 
	} // if (mesh->HasFaces()) 

	/*
	if (mesh->HasBones())
	{
		boneMap->ResetMinMax();

		// find max number of bones for vertex
		static std::vector<UINT16> bonesCounter;

		bonesCounter.resize(mesh->mNumVertices);
		memset(&bonesCounter[0], 0, bonesCounter.size() * sizeof(bonesCounter[0]));
		for (unsigned int i = 0; i < mesh->mNumBones; ++i)
		{
			aiBone * bone = mesh->mBones[i];

			UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str());
			for (unsigned int j = 0; j < bone->mNumWeights; ++j)
			{
				++bonesCounter[bone->mWeights[j].mVertexId];
			}
		}

		UINT16 max_bones_in_vert = 0;
		for (unsigned int i = 0; i < bonesCounter.size(); ++i)
		{
			if (bonesCounter[i] > max_bones_in_vert)
			{
				max_bones_in_vert = bonesCounter[i];
			}
		}
		max_bones_in_vert = ((max_bones_in_vert + 3) / 4) * 4;

		// reserver memory for bones info
		u16_boneIDs.resize(max_bones_in_vert*mesh->mNumVertices);
		f32_weights.resize(max_bones_in_vert*mesh->mNumVertices);
		memset(&u16_boneIDs[0], 0, u16_boneIDs.size() * sizeof(u16_boneIDs[0]));
		memset(&f32_weights[0], 0, f32_weights.size() * sizeof(f32_weights[0]));

		// copy data
		for (unsigned int i = 0; i < mesh->mNumBones; ++i)
		{
			aiBone * bone = mesh->mBones[i];
			UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str()) - boneMap->GetBaseID();
			for (unsigned int j = 0; j < bone->mNumWeights; ++j)
			{
				int idx = bone->mWeights[j].mVertexId;
				--bonesCounter[idx];
				int bi = idx * max_bones_in_vert + bonesCounter[idx];
				u16_boneIDs[bi] = bid;
				f32_weights[bi] = bone->mWeights[j].mWeight;
			}
		}

		// sort (eehhh... buble sort...lame)
		for (unsigned int i = 0; i < f32_weights.size(); i += max_bones_in_vert)
		{
			UINT16 *boneIDs = &u16_boneIDs[i];
			GLfloat *weights = &f32_weights[i];
			for (unsigned int j = 0; j < static_cast<unsigned int>(max_bones_in_vert - 1); ++j)
			{
				for (unsigned int k = j + 1; k < max_bones_in_vert; ++k)
				{
					if (weights[j] < weights[k]) // switch
					{
						GLfloat t = weights[j];
						weights[j] = weights[k];
						weights[k] = t;
						UINT16 t2 = boneIDs[j];
						boneIDs[j] = boneIDs[k];
						boneIDs[k] = t2;
					}
				}
			}
		}

		// erase not needed bones
		int max_bones = COUNT_OF(m_boneIDs) * 4 < GetOptimize()->max_num_of_bones_in_vertex ? COUNT_OF(m_boneIDs) * 4 : GetOptimize()->max_num_of_bones_in_vertex;

		if (max_bones_in_vert > max_bones)
		{
			for (unsigned int i = 0; i < f32_weights.size(); i += max_bones_in_vert)
			{
				GLfloat *weights = &f32_weights[i];
				GLfloat sum = 0.0f;
				for (int j = 0; j < GetOptimize()->max_num_of_bones_in_vertex; ++j)
				{
					sum += weights[j];
				}
				GLfloat invSum = 1.0f / sum;

				// fix weights
				sum = 1.0;
				for (int j = 1; j < GetOptimize()->max_num_of_bones_in_vertex; ++j)
				{
					weights[j] *= invSum;
					sum -= weights[j];
				}
				weights[0] = sum;
			}
		}

		int bones_in_vertex = max_bones;
		for (int i = 0; i < COUNT_OF(m_boneIDs); ++i)
		{
			int bones_in_streem = bones_in_vertex < 4 ? bones_in_vertex : 4;
			m_boneIDs[i] = Stream(&u16_boneIDs[i * 4], bones_in_streem, max_bones_in_vert * sizeof(u16_boneIDs[0]));
			m_boneWeights[i] = Stream(&f32_weights[i * 4], bones_in_streem, max_bones_in_vert * sizeof(f32_weights[0]));
			bones_in_vertex -= bones_in_streem;
			if (bones_in_vertex == 0)
				break;
		}
		bonesCounter.clear();
	} // if (mesh->HasBones()) 
	*/
} // VertexDescription

using namespace BAMS::CORE;

void Import::SetVD(VertexDescription &vd, aiMesh *mesh)
{
	vd = VertexDescription();

	vd.m_numVertices = mesh->mNumVertices;
	vd.m_vertices = Stream(FLOAT_3D, 3 * sizeof(F32), false);

	if (mesh->HasNormals())
		vd.m_normals = Stream(FLOAT_3D, 3 * sizeof(F32), false);

	if (mesh->HasTangentsAndBitangents()) {
		vd.m_tangents = Stream(FLOAT_3D, 3 * sizeof(F32), false);
		vd.m_bitangents = Stream(FLOAT_3D, 3 * sizeof(F32), false);
	}

	for (int i = 0; i < COUNT_OF(vd.m_colors); ++i)
	{
		if (mesh->HasVertexColors(i))
		{
			bool normalized = true;
			for (unsigned int j = 0; j < mesh->mNumVertices && normalized; ++j)
			{
				for (unsigned int k = 0; k < 4; ++k)
				{
					if (mesh->mColors[i][j][k] < 0.0 || mesh->mColors[i][j][k] > 1.0)
					{
						normalized = false;
						break;
					}
				}
			}
			vd.m_colors[i] = Stream(FLOAT_4D, 4 * sizeof(F32), normalized);
		}
	}

	for (int i = 0; i < COUNT_OF(vd.m_textureCoords); ++i)
	{
		if (mesh->HasTextureCoords(i))
		{
			bool normalized = true;
			for (unsigned int j = 0; j < mesh->mNumVertices && normalized; ++j)
			{
				for (unsigned int k = 0; k < mesh->mNumUVComponents[i]; ++k)
				{
					if (mesh->mTextureCoords[i][j][k] < 0.0 || mesh->mTextureCoords[i][j][k] > 1.0)
					{
						normalized = false;
						break;
					}
				}
			}
			auto n = mesh->mNumUVComponents[i];
			vd.m_textureCoords[i] = Stream(FLOAT_1D - 1 + n, static_cast<U16>(n * sizeof(F32)), normalized);
		}
	}

	if (mesh->HasFaces())
	{
		uint32_t faces = 0;
		bool uint16IsFine = true;
		const int MAX_UINT16 = 0xffff;

		for (unsigned int i = 0; i < mesh->mNumFaces; ++i)
		{
			if (mesh->mFaces[i].mNumIndices == 3)
			{ // only triangles
				for (unsigned int j = 0; j < mesh->mFaces[i].mNumIndices; ++j)
				{
					if (mesh->mFaces[i].mIndices[j] > MAX_UINT16)
					{
						uint16IsFine = false;
					}
				}
				++faces;
			}
		}

		if (faces)
		{
			vd.m_numIndices = faces * 3;
			if (uint16IsFine)
				vd.m_indices = Stream(IDX_UINT16_1D, sizeof(U16), false);
			else
				vd.m_indices = Stream(IDX_UINT32_1D, sizeof(U32), false);
		} // if (faces) 
	} // if (mesh->HasFaces()) 

	/*
	if (mesh->HasBones())
	{
		boneMap->ResetMinMax();

		// find max number of bones for vertex
		static std::vector<UINT16> bonesCounter;

		bonesCounter.resize(mesh->mNumVertices);
		memset(&bonesCounter[0], 0, bonesCounter.size() * sizeof(bonesCounter[0]));
		for (unsigned int i = 0; i < mesh->mNumBones; ++i)
		{
			aiBone * bone = mesh->mBones[i];

			UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str());
			for (unsigned int j = 0; j < bone->mNumWeights; ++j)
			{
				++bonesCounter[bone->mWeights[j].mVertexId];
			}
		}

		UINT16 max_bones_in_vert = 0;
		for (unsigned int i = 0; i < bonesCounter.size(); ++i)
		{
			if (bonesCounter[i] > max_bones_in_vert)
			{
				max_bones_in_vert = bonesCounter[i];
			}
		}
		max_bones_in_vert = ((max_bones_in_vert + 3) / 4) * 4;

		// reserver memory for bones info
		u16_boneIDs.resize(max_bones_in_vert*mesh->mNumVertices);
		f32_weights.resize(max_bones_in_vert*mesh->mNumVertices);
		memset(&u16_boneIDs[0], 0, u16_boneIDs.size() * sizeof(u16_boneIDs[0]));
		memset(&f32_weights[0], 0, f32_weights.size() * sizeof(f32_weights[0]));

		// copy data
		for (unsigned int i = 0; i < mesh->mNumBones; ++i)
		{
			aiBone * bone = mesh->mBones[i];
			UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str()) - boneMap->GetBaseID();
			for (unsigned int j = 0; j < bone->mNumWeights; ++j)
			{
				int idx = bone->mWeights[j].mVertexId;
				--bonesCounter[idx];
				int bi = idx * max_bones_in_vert + bonesCounter[idx];
				u16_boneIDs[bi] = bid;
				f32_weights[bi] = bone->mWeights[j].mWeight;
			}
		}

		// sort (eehhh... buble sort...lame)
		for (unsigned int i = 0; i < f32_weights.size(); i += max_bones_in_vert)
		{
			UINT16 *boneIDs = &u16_boneIDs[i];
			GLfloat *weights = &f32_weights[i];
			for (unsigned int j = 0; j < static_cast<unsigned int>(max_bones_in_vert - 1); ++j)
			{
				for (unsigned int k = j + 1; k < max_bones_in_vert; ++k)
				{
					if (weights[j] < weights[k]) // switch
					{
						GLfloat t = weights[j];
						weights[j] = weights[k];
						weights[k] = t;
						UINT16 t2 = boneIDs[j];
						boneIDs[j] = boneIDs[k];
						boneIDs[k] = t2;
					}
				}
			}
		}

		// erase not needed bones
		int max_bones = COUNT_OF(m_boneIDs) * 4 < GetOptimize()->max_num_of_bones_in_vertex ? COUNT_OF(m_boneIDs) * 4 : GetOptimize()->max_num_of_bones_in_vertex;

		if (max_bones_in_vert > max_bones)
		{
			for (unsigned int i = 0; i < f32_weights.size(); i += max_bones_in_vert)
			{
				GLfloat *weights = &f32_weights[i];
				GLfloat sum = 0.0f;
				for (int j = 0; j < GetOptimize()->max_num_of_bones_in_vertex; ++j)
				{
					sum += weights[j];
				}
				GLfloat invSum = 1.0f / sum;

				// fix weights
				sum = 1.0;
				for (int j = 1; j < GetOptimize()->max_num_of_bones_in_vertex; ++j)
				{
					weights[j] *= invSum;
					sum -= weights[j];
				}
				weights[0] = sum;
			}
		}

		int bones_in_vertex = max_bones;
		for (int i = 0; i < COUNT_OF(m_boneIDs); ++i)
		{
			int bones_in_streem = bones_in_vertex < 4 ? bones_in_vertex : 4;
			m_boneIDs[i] = Stream(&u16_boneIDs[i * 4], bones_in_streem, max_bones_in_vert * sizeof(u16_boneIDs[0]));
			m_boneWeights[i] = Stream(&f32_weights[i * 4], bones_in_streem, max_bones_in_vert * sizeof(f32_weights[0]));
			bones_in_vertex -= bones_in_streem;
			if (bones_in_vertex == 0)
				break;
		}
		bonesCounter.clear();
	} // if (mesh->HasBones())
	*/
} // VertexDescription

void Import::LoadMeshes()
{
	const char *p = _fileName.c_str();
	for (const char *f = p; *f; ++f) 
	{
		if (*f == '\\' || *f == '/')
			p = f + 1;
	}
	std::string path(_fileName.c_str(), p); // path to file

	// loading meshes
	uint32_t numValidMeshes = 0;
	for (unsigned int i = 0; i < _pScene->mNumMeshes; ++i)
	{
		aiMesh *srcMesh = _pScene->mMeshes[i];
		if (srcMesh->mNumVertices)
			++numValidMeshes;
	}

	uint32_t validMeshID = 0;
	for (unsigned int i = 0; i < _pScene->mNumMeshes; ++i) {
		aiMesh *srcMesh = _pScene->mMeshes[i];
		if (srcMesh->mNumVertices == 0) // skip meshes without vertices (empty)
			continue;

		BAMS::RENDERINENGINE::VertexDescription vd;
		SetVD(vd, srcMesh);
/*
		m->m_pMeshes[validMeshID] = new MeshToDrawFactory();
		m->m_pMeshes[validMeshID]->m_pMesh = VertexManager::AddMesh(srcMesh, m_pBones);
		if (m_importMaterials)
		{
			m_pMaterials[validMeshID] = pMaterials[srcMesh->mMaterialIndex];
		}

		if (!minMaxVertSet && srcMesh->mNumVertices > 0) {
			if (_pScene->mNumMeshes > 0) {
				for (int k = 0; k < 3; ++k)
					m->minVert[k] = m->maxVert[k] = srcMesh->mVertices[0][k];
				minMaxVertSet = true;
			}
		}

		aiVector3D *v = srcMesh->mVertices;
		for (unsigned int j = 0; j < srcMesh->mNumVertices; ++j) {
			for (int k = 0; k < 3; ++k) {
				if (m->minVert[k] > v[j][k])
					m->minVert[k] = v[j][k];
				if (m->maxVert[k] < v[j][k])
					m->maxVert[k] = v[j][k];
			}
		}

		if (!minMaxTexSet && srcMesh->mNumVertices > 0) {
			for (int k = 0; k < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++k) {
				if (srcMesh->HasTextureCoords(k)) {
					minMaxTexSet = true;
					for (int l = 0; l < 3; ++l)
						minTex[l] = maxTex[l] = srcMesh->mTextureCoords[k][0][l];
					minMaxTexSet = true;
					break;
				}
			}
		}

		for (int k = 0; k < AI_MAX_NUMBER_OF_TEXTURECOORDS; ++k) {
			if (srcMesh->HasTextureCoords(k)) {
				aiVector3D *v = srcMesh->mTextureCoords[k];
				for (unsigned int j = 0; j < srcMesh->mNumVertices; ++j) {
					for (int l = 0; l < 3; ++l) {
						if (minTex[l] > v[j][l])
							minTex[l] = v[j][l];
						if (maxTex[l] < v[j][l])
							maxTex[l] = v[j][l];
					}
				}
			}
		}
		*/
		++validMeshID;
	}
}

void Import::LoadAnims()
{
	if (!_pScene->HasAnimations())
		return;
}

void Import::FillBoneTree(const aiNode * pNode, int parentNodeID)
{
}

void Import::ProcessScene()
{
	if (!_pScene)
		return;

	LoadBones();
	LoadMeshes();
	LoadAnims();
}
