
enum {
	NONE = 0,
	FLOATS_TO_HALF_FLOATS = 1,
	FLOATS_TO_INT16 = 2,
	FLOATS_TO_UINT16 = 3,
	FLOATS_TO_INT8 = 4,
	FLOATS_TO_UINT8 = 5,
	FLOATS_TO_PACKED_INT10 = 6,
	FLOATS_TO_PACKED_UINT10 = 7,
	UINT16_TO_UINT8 = 8
};

enum {
	POW2 = 1,
	MOD4 = 2
};

struct VertexDescription;

struct BAMS_EXPORT Optimize {
	Optimize();
	VertexDescription OptimizeVertexDescription(VertexDescription &vd);
	uint32_t GetOptimizedType(uint32_t type, uint32_t size, bool normalized = true);

	U32 normals;							// normals, tangents, bitangents
	U32 stride;
	U32 colors;
	U32 coords;
	U32 bone_weights;
	U32 bone_ids;
	bool delete_bitangents;
	bool force_32bit_indices;
	U32 max_texture_size;
	U32 max_textures_array_size;
	U32 max_textures_array_depth;
	U32 max_texture_array_memory_size;
	U32 max_VAO_data_memory_size;
	U32 max_VAO_idx_data_memory_size;
	U32 max_num_of_bones_in_vertex;
	float max_smoothing_angle;
};

Optimize BAMS_EXPORT *GetOptimize();
