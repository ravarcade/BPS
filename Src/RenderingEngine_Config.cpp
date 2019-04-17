#include "stdafx.h"

NAMESPACE_RENDERINENGINE_BEGIN

enum TUsage {
	POSITION = 0,
	NORMALIZED = 1,
	TEXTURECOORD = 2,
	COLOR = 3,
	WEIGHT = 4,
	INDICES = 5,
	BITANGENT = 6,
	BONE_ID = 7,
	BONE_WEIGHT = 8
};

U16 Stream::typeOptimalStride[MAX_TYPE] = {
		0,  // UNUSED = 0,
		4,  // FLOAT_1D = 1,
		8,  // FLOAT_2D = 2,
		12, // FLOAT_3D = 3,
		16, // FLOAT_4D = 4,
		4,  // COL_UINT8_4D = 5, // colors
		2,  // IDX_UINT16_1D = 6, // index buffer
		4,  // IDX_UINT32_1D = 7, // index buffer
		2,  // HALF_1D = 8,
		4,  // HALF_2D = 9,
		6,  // HALF_3D = 10,
		8,  // HALF_4D = 11,

		// used for normals/tangents 
		// N_ = normalized
		2,  // N_SHORT_1D = 12,
		4,  // N_SHORT_2D = 13,
		6,  // N_SHORT_3D = 14,
		8,  // N_SHORT_4D = 15, // not needed

		// used for colors
		2,  // N_USHORT_1D = 16,
		4,  // N_USHORT_2D = 17,
		6,  // N_USHORT_3D = 18,
		8,  // N_USHORT_4D = 19, // not needed

		// packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
		4,  // INT_PACKED = 20,
		4,  // UINT_PACKED = 21,

		2,  // UINT16_1D = 22,
		4,  // UINT16_2D = 23,
		6,  // UINT16_3D = 24,
		8,  // UINT16_4D = 25,

		2,  // UINT8_1D = 26,
		4,  // UINT8_2D = 27,
		6,  // UINT8_3D = 28,
		8,  // UINT8_4D = 29,
};

Optimize::Optimize() 
{
	// ================================================ optimalization defaults ===
	// normals/tangents/bitangents (normalized 3D floats vectors) stored as: (1) 3x FLOATS, (2) 3x INT16, (3) PACKED INT10 in UINT32

	normals = FLOATS_TO_PACKED_INT10;

	// colors if posible save as UINT32
	colors = FLOATS_TO_UINT8;
//	colors = FLOATS_TO_INT16;

	// texture coords in range <0,1> will be stored as UINT16
	coords = FLOATS_TO_UINT16;

	// vertex stride will be 4 bytes aligned
	stride = MOD4;

	delete_bitangents = true;						// we want to delete bitangents (it will be calculated in shader)
	force_32bit_indices = false;				   // accept 16 bit indices


	max_texture_size = 2048;

	max_textures_array_depth = 256;
	max_textures_array_size = 2048;
	max_texture_array_memory_size = 128 * 1024 * 1024;		// 128 MB

	max_VAO_data_memory_size = 16 * 1024 * 1024;			// 16 MB
	max_VAO_idx_data_memory_size = 4 * 1024 * 1024;			// 4 MB

	max_num_of_bones_in_vertex = 4;
	bone_weights = FLOATS_TO_UINT8;
	bone_ids = UINT16_TO_UINT8;
}

UINT32 vertex_attrib_type(TUsage usage, UINT32 type, bool normalized)
{
	if (type == UNUSED)
	{
		return UNUSED;
	}
	if (type < FLOAT_1D && type > FLOAT_4D)
		return type;

	switch (usage) {
	case POSITION:
		return type;

	case INDICES: // always use 32bit indices ?
		return GetOptimize()->force_32bit_indices ? IDX_UINT32_1D : type;

	case BITANGENT: // remove bitangents?
		if (GetOptimize()->delete_bitangents)
			return UNUSED;

	case NORMALIZED:
		switch (GetOptimize()->normals) {
		case FLOATS_TO_HALF_FLOATS:  return type - (FLOAT_1D - HALF_1D);
		case FLOATS_TO_INT16:        return type - (FLOAT_1D - N_SHORT_1D);
		case FLOATS_TO_PACKED_INT10: return (type - FLOAT_1D + 1) < 4 ? INT_PACKED : type;
		}
		break;

	case COLOR:
		switch (GetOptimize()->colors) {
		case FLOATS_TO_HALF_FLOATS:   return type - (FLOAT_1D - HALF_1D);
		case FLOATS_TO_UINT8:         return normalized ? COL_UINT8_4D : type;
		case FLOATS_TO_UINT16:        return normalized ? type - (FLOAT_1D - N_USHORT_1D) : type;
		case FLOATS_TO_PACKED_UINT10: return normalized ? UINT_PACKED : type;
		}
		break;

	case TEXTURECOORD:
		switch (GetOptimize()->coords) {
		case FLOATS_TO_HALF_FLOATS:   return type - (FLOAT_1D - HALF_1D);
		case FLOATS_TO_UINT16:        return normalized ? type - (FLOAT_1D - N_USHORT_1D) : type;
		case FLOATS_TO_PACKED_UINT10: return normalized ? UINT_PACKED : type;
		}
		break;

	case BONE_WEIGHT: // like color
		switch (GetOptimize()->bone_weights) {
		case FLOATS_TO_HALF_FLOATS:   return type - (FLOAT_1D - HALF_1D);
		case FLOATS_TO_UINT8:         return COL_UINT8_4D;
		case FLOATS_TO_UINT16:        return normalized ? type - (FLOAT_1D - N_USHORT_1D) : type;
		}
		break;

	case BONE_ID:
		switch (GetOptimize()->bone_ids) {
		case UINT16_TO_UINT8:         return UINT8_4D;
		}
		break;
	}

	return type;
}

VertexDescription Optimize::OptimizeVertexDescription(VertexDescription &vd)
{
	VertexDescription o;

	o.m_vertices = vd.m_vertices;
#define OPTIM(x,y) { o.m_##y.m_type = vertex_attrib_type(x, vd.m_##y.m_type, vd.m_##y.m_normalized); o.m_##y.m_normalized = vd.m_##y.m_normalized; }

	OPTIM(INDICES, indices);
	OPTIM(NORMALIZED, normals);
	OPTIM(NORMALIZED, tangents);
	OPTIM(BITANGENT, bitangents);
	for (uint32_t i = 0; i < COUNT_OF(o.m_colors); ++i)
		OPTIM(COLOR, colors[i]);
	for (uint32_t i = 0; i < COUNT_OF(o.m_textureCoords); ++i)
		OPTIM(TEXTURECOORD, textureCoords[i]);
	for (uint32_t i = 0; i < COUNT_OF(o.m_boneIDs); ++i)
		OPTIM(BONE_ID, boneIDs[i]);
	for (uint32_t i = 0; i < COUNT_OF(o.m_boneWeights); ++i)
		OPTIM(BONE_WEIGHT, boneWeights[i]);

	return o;
}

Optimize *GetOptimize()
{
	static Optimize optimize;

	return &optimize;
}

NAMESPACE_RENDERINENGINE_END