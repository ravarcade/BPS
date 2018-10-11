#ifndef _RENDERINGENGINE_VERTEXDESCRIPTION_H_
#define _RENDERINGENGINE_VERTEXDESCRIPTION_H_

const int MAX_NUM_OF_TEX_COORDS = 4;
const int MAX_NUM_OF_COLORS = 4;
const int MAX_NUM_OF_BONES_IN_VERT = 8 / 4;

/**
* enum: Constants definition
*/
enum {
	UNUSED = 0,
	FLOAT_1D = 1,
	FLOAT_2D = 2,
	FLOAT_3D = 3,
	FLOAT_4D = 4,
	COL_UINT8_4D = 5, // colors
	IDX_UINT16_1D = 6, // index buffer
	IDX_UINT32_1D = 7, // index buffer
	HALF_1D = 8,
	HALF_2D = 9,
	HALF_3D = 10,
	HALF_4D = 11,


	// used for normals/tangents 
	// N_ = normalized
	N_SHORT_1D = 12,
	N_SHORT_2D = 13,
	N_SHORT_3D = 14,
	N_SHORT_4D = 15, // not needed

	// used for colors
	N_USHORT_1D = 16,
	N_USHORT_2D = 17,
	N_USHORT_3D = 18,
	N_USHORT_4D = 19, // not needed

	// packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
	INT_PACKED = 20,
	UINT_PACKED = 21,

	UINT16_1D = 22,
	UINT16_2D = 23,
	UINT16_3D = 24,
	UINT16_4D = 25,

	UINT8_1D = 26,
	UINT8_2D = 27,
	UINT8_3D = 28,
	UINT8_4D = 29,

	MAX_TYPE = 30
};

/**
* Desction of single data information stream.
* Used internaly only
*/
struct RE_EXPORT Stream {
	void *m_data;
	U16 m_stride;
	U32 m_type;
	bool m_normalized;

	Stream() :
		m_data(0),
		m_stride(0),
		m_type(UNUSED),
		m_normalized(false)
	{};

	Stream(F32 *data, U16 len = 3, U16 stride = 0) :
		m_data(data),
		m_type(FLOAT_1D - 1 + len),
		m_stride(stride == 0 ? sizeof(data[0])*len : stride),
		m_normalized(false)
	{};


	Stream(DWORD *data, U16 len = 4, U16 stride = 0) :
		m_data(data),
		m_type(COL_UINT8_4D),
		m_stride(stride == 0 ? sizeof(U8)*len : stride),
		m_normalized(false)
	{};

	Stream(U8 *data, U16 len = 1, U16 stride = 0) :
		m_data(data),
		m_type(UINT8_1D - 1 + len),
		m_stride(stride == 0 ? sizeof(data[0])*len : stride),
		m_normalized(false)
	{};


	// len = 0 means index
	Stream(U16 *data, U16 len = 0, U16 stride = 0) :
		m_data(data),
		m_type(len == 0 ? IDX_UINT16_1D : UINT16_1D - 1 + len),
		m_stride(stride == 0 ? sizeof(data[0])*(len == 0 ? 1 : len) : stride),
		m_normalized(false)
	{};

	Stream(U32 *data, U16 len = 1, U16 stride = 0) :
		m_data(data),
		m_type(IDX_UINT32_1D),
		m_stride(stride == 0 ? sizeof(data[0]) : stride),
		m_normalized(false)
	{};

	bool isUsed() { return m_type != 0; }
};

/**
*  Definition of input vertex data structure
*  Here is also translation form assimp::aiMesh to my vertex data structure
*/
struct RE_EXPORT VertexDescription
{
public:
	U32 m_numVertices;
	U32 m_numIndices;

	Stream m_vertices, m_normals, m_tangents, m_bitangents;
	Stream m_colors[MAX_NUM_OF_COLORS];
	Stream m_textureCoords[MAX_NUM_OF_TEX_COORDS];
	Stream m_indices;

	// max 4 bones
	Stream m_boneWeights[MAX_NUM_OF_BONES_IN_VERT];
	Stream m_boneIDs[MAX_NUM_OF_BONES_IN_VERT];


	VertexDescription(
		U32 numTriangles, 
		U32 numVertices, 
		F32 *pVertices, 
		U32 *pIndices,
		F32 *pTexCoords = nullptr,
		F32 *pNormals = nullptr,
		F32 *pTangents = nullptr,
		F32 *pBitangents = nullptr, 
		F32 *pColors = nullptr)
	{
		m_numVertices = numVertices;
		m_numIndices = numTriangles * 3;

		m_vertices = Stream(pVertices);
		if (pNormals)
			m_normals = Stream(pNormals);

		if (pTangents)
			m_tangents = Stream(pTangents);

		if (pBitangents)
			m_bitangents = Stream(pBitangents);
		
		if (pColors)
		{
			uint32_t i = 0;
			m_colors[i] = Stream(pColors, 4);
			m_colors[i].m_normalized = true;
			for (unsigned int j = 0; j < numVertices * 4; ++j)
			{
				if (pColors[j] < 0.0 || pColors[j] > 1.0)
				{
					m_colors[i].m_normalized = false;
					break;
				}
			}
		}

		if (pTexCoords)
		{
			uint32_t i = 0;
			m_colors[i] = Stream(pTexCoords, 2);
			m_colors[i].m_normalized = true;
			for (unsigned int j = 0; j < numVertices * 2; ++j)
			{
				if (pTexCoords[j] < 0.0 || pTexCoords[j] > 1.0)
				{
					m_colors[i].m_normalized = false;
					break;
				}
			}
		}


		m_indices = Stream(pIndices,1);
		bool uint16IsFine = true;
		const int MAX_UINT16 = 0xffff;

		for (unsigned int i = 0; i < m_numIndices; ++i)
		{
			if (pIndices[i] > MAX_UINT16)
			{
				uint16IsFine = false;
				break;
			}
		}
		if (uint16IsFine)
		{
			m_indices.m_type = IDX_UINT16_1D;			
		}

	}

	/*
	VertexDescription(const aiMesh *mesh, BonesFactory *boneMap)
	{
		m_numVertices = mesh->mNumVertices;
		m_vertices = Stream(&mesh->mVertices[0][0]);

		if (mesh->HasNormals())
			m_normals = Stream(&mesh->mNormals[0][0]);

		if (mesh->HasTangentsAndBitangents()) {
			m_tangents = Stream(&mesh->mTangents[0][0]);
			m_bitangents = Stream(&mesh->mBitangents[0][0]);
		}

		for (int i = 0; i<COUNT_OF(m_colors); ++i)
		{
			if (mesh->HasVertexColors(i))
			{
				bool normalized = true;
				for (unsigned int j = 0; j<mesh->mNumVertices && normalized; ++j)
				{
					for (unsigned int k = 0; k<4; ++k)
					{
						if (mesh->mColors[i][j][k] < 0.0 || mesh->mColors[i][j][k] > 1.0)
						{
							normalized = false;
							break;
						}
					}
				}
				m_colors[i] = Stream(&mesh->mColors[i][0][0], 4);
				m_colors[i].m_normalized = normalized;
			}
		}

		for (int i = 0; i<COUNT_OF(m_textureCoords); ++i)
		{
			if (mesh->HasTextureCoords(i))
			{
				bool normalized = true;
				for (unsigned int j = 0; j<mesh->mNumVertices && normalized; ++j)
				{
					for (unsigned int k = 0; k< mesh->mNumUVComponents[i]; ++k)
					{
						if (mesh->mTextureCoords[i][j][k] < 0.0 || mesh->mTextureCoords[i][j][k] > 1.0)
						{
							normalized = false;
							break;
						}
					}
				}
				m_textureCoords[i] = Stream(&mesh->mTextureCoords[i][0][0], mesh->mNumUVComponents[i], sizeof(mesh->mTextureCoords[i][0][0]) * 3);
				m_textureCoords[i].m_normalized = normalized;
			}
		}

		if (mesh->HasFaces())
		{
			unsigned int faces = 0;
			bool uint16IsFine = true;
			const int MAX_UINT16 = 0xffff;

			for (unsigned int i = 0; i<mesh->mNumFaces; ++i)
			{
				if (mesh->mFaces[i].mNumIndices == 3)
				{ // only triangles
					for (unsigned int j = 0; j<mesh->mFaces[i].mNumIndices; ++j)
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
				m_numIndices = faces * 3;
				if (uint16IsFine)
				{
					u16_indicesData.resize(faces * 3);
					m_indices = Stream(&u16_indicesData[0], 0); // it is index, so len = 0
					UINT16 *p = (UINT16 *)m_indices.m_data;
					for (unsigned int i = 0; i<mesh->mNumFaces; ++i)
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
					m_indices = Stream(&u32_indicesData[0], 0); // it is index, so len = 0
					UINT32 *p = (UINT32 *)m_indices.m_data;
					for (unsigned int i = 0; i<mesh->mNumFaces; ++i)
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

		if (mesh->HasBones())
		{
			boneMap->ResetMinMax();

			// find max number of bones for vertex
			static std::vector<UINT16> bonesCounter;

			bonesCounter.resize(mesh->mNumVertices);
			memset(&bonesCounter[0], 0, bonesCounter.size() * sizeof(bonesCounter[0]));
			for (unsigned int i = 0; i<mesh->mNumBones; ++i)
			{
				aiBone * bone = mesh->mBones[i];

				UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str());
				for (unsigned int j = 0; j<bone->mNumWeights; ++j)
				{
					++bonesCounter[bone->mWeights[j].mVertexId];
				}
			}

			UINT16 max_bones_in_vert = 0;
			for (unsigned int i = 0; i<bonesCounter.size(); ++i)
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
			for (unsigned int i = 0; i<mesh->mNumBones; ++i)
			{
				aiBone * bone = mesh->mBones[i];
				UINT16 bid = boneMap->GetBoneID(bone->mName.C_Str()) - boneMap->GetBaseID();
				for (unsigned int j = 0; j<bone->mNumWeights; ++j)
				{
					int idx = bone->mWeights[j].mVertexId;
					--bonesCounter[idx];
					int bi = idx * max_bones_in_vert + bonesCounter[idx];
					u16_boneIDs[bi] = bid;
					f32_weights[bi] = bone->mWeights[j].mWeight;
				}
			}

			// sort (eehhh... buble sort...lame)
			for (unsigned int i = 0; i<f32_weights.size(); i += max_bones_in_vert)
			{
				UINT16 *boneIDs = &u16_boneIDs[i];
				GLfloat *weights = &f32_weights[i];
				for (unsigned int j = 0; j<static_cast<unsigned int>(max_bones_in_vert - 1); ++j)
				{
					for (unsigned int k = j + 1; k<max_bones_in_vert; ++k)
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
				for (unsigned int i = 0; i<f32_weights.size(); i += max_bones_in_vert)
				{
					GLfloat *weights = &f32_weights[i];
					GLfloat sum = 0.0f;
					for (int j = 0; j<GetOptimize()->max_num_of_bones_in_vertex; ++j)
					{
						sum += weights[j];
					}
					GLfloat invSum = 1.0f / sum;

					// fix weights
					sum = 1.0;
					for (int j = 1; j<GetOptimize()->max_num_of_bones_in_vertex; ++j)
					{
						weights[j] *= invSum;
						sum -= weights[j];
					}
					weights[0] = sum;
				}
			}

			int bones_in_vertex = max_bones;
			for (int i = 0; i<COUNT_OF(m_boneIDs); ++i)
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
	} // VertexDescription
	*/
};

#endif // _RENDERINGENGINE_VERTEXDESCRIPTION_H_