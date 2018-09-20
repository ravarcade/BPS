#include "stdafx.h"

NAMESPACE_RENDERINENGINE_BEGIN

Optimize::Optimize() 
{
	// ================================================ optimalization defaults ===
	// normals/tangents/bitangents (normalized 3D floats vectors) stored as: (1) 3x FLOATS, (2) 3x INT16, (3) PACKED INT10 in UINT32

	normals = FLOATS_TO_PACKED_INT10;

	// colors if posible save as UINT32
	colors = FLOATS_TO_UINT8;

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

Optimize *GetOptimize()
{
	static Optimize optimize;

	return &optimize;
}

NAMESPACE_RENDERINENGINE_END