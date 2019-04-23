#include "stdafx.h"

NAMESPACE_RENDERINENGINE_BEGIN

Stream::Desc Stream::TypeDescription[VAT_MAX_TYPE] = {
	{  0, 0, false},  // UNUSED = 0,
	{  4, 1, false},  // FLOAT_1D = 1,
	{  8, 2, false},  // FLOAT_2D = 2,
	{ 12, 3, false},  // FLOAT_3D = 3,
    { 16, 4, false},  // FLOAT_4D = 4,
    {  4, 4,  true},  // COL_UINT8_4D = 5, // colors
    {  2, 1, false},  // IDX_UINT16_1D = 6, // index buffer
    {  4, 1, false},  // IDX_UINT32_1D = 7, // index buffer
	{  2, 1, false},  // HALF_1D = 8,
	{  4, 2, false},  // HALF_2D = 9,
    {  6, 3, false},  // HALF_3D = 10,
    {  8, 4, false},  // HALF_4D = 11,

		// used for normals/tangents/texcoords
		// N_ = normalized
	{  2, 1,  true},  // N_SHORT_1D = 12,
	{  4, 2,  true},  // N_SHORT_2D = 13,
	{  6, 3,  true},  // N_SHORT_3D = 14,
	{  8, 4,  true},  // N_SHORT_4D = 15, // not needed

		// used for colors/texcoords
	{  2, 1,  true},  // N_USHORT_1D = 16,
	{  4, 2,  true},  // N_USHORT_2D = 17,
	{  6, 3,  true},  // N_USHORT_3D = 18,
	{  8, 4,  true},  // N_USHORT_4D = 19, // not needed

		// packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
	{  4, 4,  true},  // INT_PACKED = 20,
	{  4, 4,  true},  // UINT_PACKED = 21,

	{  2, 1, false},  // UINT16_1D = 22,
	{  4, 2, false},  // UINT16_2D = 23,
	{  6, 3, false},  // UINT16_3D = 24,
	{  8, 4, false},  // UINT16_4D = 25,

	{  2, 1, false},  // UINT8_1D = 26,
	{  4, 2, false},  // UINT8_2D = 27,
	{  6, 3, false},  // UINT8_3D = 28,
	{  8, 4, false},  // UINT8_4D = 29,
};

Optimize::Optimize() 
{
	// ================================================ optimalization defaults ===
	// normals/tangents/bitangents (normalized 3D floats vectors) stored as: (1) 3x FLOATS, (2) 3x INT16, (3) PACKED INT10 in UINT32

	normals = FLOATS_TO_PACKED_INT10;

	// colors if posible save as UINT32
	colors = FLOATS_TO_UINT8;

	// texture coords in range <0,1> will be stored as UINT16
	coords = NONE;

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

VertexDescription Optimize::OptimizeVertexDescription(VertexDescription &vd)
{
	VertexDescription o;

	o.m_numVertices = vd.m_numVertices;
	o.m_numIndices = vd.m_numIndices;
	o.m_vertices = vd.m_vertices;
	uint32_t stride = 0;
	for (uint32_t i = VA_POSITION; i <= VA_BONEID2; ++i)
	{
		auto d = o.GetStream(i);
		auto s = vd.GetStream(i);
		d->m_type = s->m_type;
		d->m_normalized = s->m_normalized;
		if (d->m_type != VAT_UNUSED)
		{
			d->m_type = GetOptimizedType(i, Stream::TypeDescription[d->m_type].count, d->m_normalized);
			d->m_normalized = d->GetDesc().normalized;
			stride += d->GetDesc().size;
		}
	}

	// set stride	
	o.ForEachStream([&](auto &s) { if (s.m_type != VAT_UNUSED) s.m_stride = stride; });
	o.m_indices = vd.m_indices;
	
	return o;
}


uint32_t Optimize::GetOptimizedType(uint32_t type, uint32_t count, bool normalized)
{
	switch (type) 
	{
	case VA_POSITION:	
		assert(1 < count && count < 4);
		return VAT_FLOAT_1D + count - 1;

	case VA_BITANGENT:  // remove bitangents?
		if (delete_bitangents)
			return VAT_UNUSED;

	case VA_NORMAL:
	case VA_TANGENT:
		switch (normals) {
		case FLOATS_TO_HALF_FLOATS:  return VAT_HALF_1D + count - 1;
		case FLOATS_TO_INT16:        return VAT_N_SHORT_1D + count - 1;
		case FLOATS_TO_PACKED_INT10: return VAT_INT_PACKED;
		}
		break;


	case VA_TEXCOORD:
	case VA_TEXCOORD2:
	case VA_TEXCOORD3:
	case VA_TEXCOORD4:
		switch (coords) {
		case FLOATS_TO_HALF_FLOATS:   return VAT_HALF_1D + count - 1;
		case FLOATS_TO_INT16:         return (normalized ? VAT_N_SHORT_1D : VAT_FLOAT_1D) + count - 1;
		case FLOATS_TO_UINT16:        return (normalized ? VAT_N_USHORT_1D : VAT_FLOAT_1D) + count - 1;
		case FLOATS_TO_PACKED_UINT10: return normalized ? VAT_UINT_PACKED : VAT_FLOAT_1D + count - 1;
		}
		break;

	case VA_COLOR:
	case VA_COLOR2:
	case VA_COLOR3:
	case VA_COLOR4:
		switch (colors) {
		case FLOATS_TO_HALF_FLOATS:   return VAT_HALF_1D + count - 1;
		case FLOATS_TO_UINT8:         return normalized ? VAT_COL_UINT8_4D : VAT_FLOAT_1D + count - 1;
		case FLOATS_TO_UINT16:        return (normalized ? VAT_N_USHORT_1D : VAT_FLOAT_1D) + count - 1;
		case FLOATS_TO_PACKED_UINT10: return normalized ? VAT_UINT_PACKED : VAT_FLOAT_1D + count - 1;
		}
		break;

	case VA_BONEWEIGHT:
	case VA_BONEWEIGHT2:
		switch (bone_weights) {
		case FLOATS_TO_HALF_FLOATS:   return VAT_HALF_1D + count - 1;
		case FLOATS_TO_UINT8:         return VAT_COL_UINT8_4D;
		case FLOATS_TO_UINT16:        return VAT_N_USHORT_1D + count - 1;
		}
		break;

	case VA_BONEID:
	case VA_BONEID2:
		return bone_ids == UINT16_TO_UINT8 ? VAT_UINT8_4D : VAT_UINT16_1D + count - 1;

	case VA_INDICES: // always use 32bit indices?
		return force_32bit_indices ? VAT_IDX_UINT32_1D : type;

	}

	
	return VAT_FLOAT_1D + count - 1;
}

Optimize *GetOptimize()
{
	static Optimize optimize;

	return &optimize;
}

NAMESPACE_RENDERINENGINE_END