#include "stdafx.h"

NAMESPACE_RENDERINENGINE_BEGIN

struct StreamTypeDataDescription {
	uint8_t len;
};

StreamTypeDataDescription stdd[MAX_TYPE] = {
	{ 0 }, // UNUSED = 0,
	{ 4 }, // FLOAT_1D = 1,
	{ 8 }, // FLOAT_2D = 2,
	{ 12 }, // FLOAT_3D = 3,
	{ 16 }, // FLOAT_4D = 4,
	{ 4 }, // COL_UINT8_4D = 5, // colors
	{ 2 }, // IDX_UINT16_1D = 6, // index buffer
	{ 4 }, // IDX_UINT32_1D = 7, // index buffer
	{ 2 }, // HALF_1D = 8,
	{ 4 }, // HALF_2D = 9,
	{ 6 }, // HALF_3D = 10,
	{ 8 }, // HALF_4D = 11,


	// used for normals/tangents 
	// N_ = normalized
	{ 2 }, // N_SHORT_1D = 12,
	{ 4 }, // N_SHORT_2D = 13,
	{ 6 }, // N_SHORT_3D = 14,
	{ 8 }, // N_SHORT_4D = 15, // not needed

	// used for colors
	{ 2 }, // N_USHORT_1D = 16,
	{ 4 }, // N_USHORT_2D = 17,
	{ 6 }, // N_USHORT_3D = 18,
	{ 8 }, // N_USHORT_4D = 19, // not needed

	// packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
	{ 4 }, // INT_PACKED = 20,
	{ 4 }, // UINT_PACKED = 21,

	{ 2 }, // UINT16_1D = 22,
	{ 4 }, // UINT16_2D = 23,
	{ 6 }, // UINT16_3D = 24,
	{ 8 }, // UINT16_4D = 25,

	{ 1 }, // UINT8_1D = 26,
	{ 2 }, // UINT8_2D = 27,
	{ 3 }, // UINT8_3D = 28,
	{ 4 }, // UINT8_4D = 29,
};

uint8_t len2mod4[] = { 0, 4, 4, 4, 4, 8, 8, 8, 8, 12, 12, 12, 12, 16, 16, 16, 16 };

uint32_t VertexDescription::GetStride()
{
	uint32_t s = 0;
	auto getLen = [](const Stream &st) { return len2mod4[stdd[st.m_type].len]; };
	
	s += getLen(m_vertices);
	s += getLen(m_normals);
	s += getLen(m_tangents);
	s += getLen(m_bitangents);
	for (auto c : m_colors)
		s += getLen(c);
	for (auto c : m_textureCoords)
		s += getLen(c);
	for (auto c : m_boneWeights)
		s += getLen(c);
	for (auto c : m_boneIDs)
		s += getLen(c);

	return s;
}

NAMESPACE_RENDERINENGINE_END

