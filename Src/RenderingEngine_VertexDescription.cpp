#include "stdafx.h"
#include <glm\glm.hpp>

namespace BAMS {

enum STDDT {
	TF32, TF16, TU32, TI16, TU16, TU8, TUPACK, TIPACK
};
struct StreamTypeDataDescription {
	uint8_t len;
	uint8_t parts;
	STDDT type;
};

StreamTypeDataDescription stdd[VAT_MAX_TYPE] = {
	{  0, 0, TF32 }, // UNUSED = 0,
	{  4, 1, TF32 }, // FLOAT_1D = 1,
	{  8, 2, TF32 }, // FLOAT_2D = 2,
	{ 12, 3, TF32 }, // FLOAT_3D = 3,
	{ 16, 4, TF32 }, // FLOAT_4D = 4,
	{  4, 4,  TU8 }, // COL_UINT8_4D = 5, // colors
	{  2, 1, TU16 }, // IDX_UINT16_1D = 6, // index buffer
	{  4, 1, TU32 }, // IDX_UINT32_1D = 7, // index buffer
	{  2, 1, TF16 }, // HALF_1D = 8,
	{  4, 2, TF16 }, // HALF_2D = 9,
	{  6, 3, TF16 }, // HALF_3D = 10,
	{  8, 4, TF16 }, // HALF_4D = 11,


	// used for normals/tangents 
	// N_ = normalized
	{  2, 1, TI16 }, // N_SHORT_1D = 12,
	{  4, 2, TI16 }, // N_SHORT_2D = 13,
	{  6, 3, TI16 }, // N_SHORT_3D = 14,
	{  8, 4, TI16 }, // N_SHORT_4D = 15, // not needed

	// used for colors
	{  2, 1, TU16 }, // N_USHORT_1D = 16,
	{  4, 2, TU16 }, // N_USHORT_2D = 17,
	{  6, 3, TU16 }, // N_USHORT_3D = 18,
	{  8, 4, TU16 }, // N_USHORT_4D = 19, // not needed

	// packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
	{  4, 1, TIPACK }, // INT_PACKED = 20,
	{  4, 1, TUPACK }, // UINT_PACKED = 21,

	{  2, 1, TU16 }, // UINT16_1D = 22,
	{  4, 2, TU16 }, // UINT16_2D = 23,
	{  6, 3, TU16 }, // UINT16_3D = 24,
	{  8, 4, TU16 }, // UINT16_4D = 25,

	{  1, 1,  TU8 }, // UINT8_1D = 26,
	{  2, 2,  TU8 }, // UINT8_2D = 27,
	{  3, 3,  TU8 }, // UINT8_3D = 28,
	{  4, 4,  TU8 }, // UINT8_4D = 29,
};

uint32_t VertexDescription::StreamLen(const Stream &s) { return (stdd[s.m_type].len + 3) &0xfffc; }

uint32_t VertexDescription::GetStride()
{
	uint32_t stride = 0;
	ForEachStream([&](const Stream &s) { stride += StreamLen(s); });
	
	return stride;
}

void VertexDescription::Copy(Stream &dst, const Stream &src)
{
	if (dst.m_type == VAT_UNUSED)
		return;

	auto d = reinterpret_cast<uint8_t*>(dst.m_data);
	auto ds = dst.m_stride;
	auto dl = StreamLen(dst);
	if (src.m_type == VAT_UNUSED) // set zero as default
	{
		auto memlen = StreamLen(dst);
		for (uint32_t i = 0; i < m_numVertices; ++i) 
		{
			memset(d, 0, dl);
			d += ds;
		}
	}
	else if (src.m_type == dst.m_type) // no conversion needed
	{ 
		auto s = reinterpret_cast<uint8_t*>(src.m_data);
		auto ss = src.m_stride;
		for (unsigned int i = 0; i < m_numVertices; ++i)
		{
			memcpy(d, s, dl);
			d += ds;
			s += ss;
		}
	}
	else if (src.m_type == VAT_IDX_UINT16_1D && dst.m_type == VAT_IDX_UINT32_1D)
	{
		auto d = reinterpret_cast<uint32_t*>(dst.m_data);
		auto s = reinterpret_cast<uint16_t*>(src.m_data);
		for (unsigned int i = 0; i < m_numIndices; ++i) 
		{
			*d = *s;
			++d;
			++s;
		}
	}
	else if (src.m_type == VAT_IDX_UINT32_1D && dst.m_type == VAT_IDX_UINT16_1D)
	{
		auto d = reinterpret_cast<uint16_t*>(dst.m_data);
		auto s = reinterpret_cast<uint32_t*>(src.m_data);
		for (unsigned int i = 0; i < m_numIndices; ++i)
		{
			*d = static_cast<uint16_t>(*s);
			++d;
			++s;
		}
	}
	else {
		// we convert all to 4x GLfloat
		auto dn = dst.m_normalized;
		auto s = reinterpret_cast<uint8_t*>(src.m_data);
		auto ss = src.m_stride;
		auto sl = StreamLen(src);
		auto sn = src.m_normalized;
		auto st = stdd[src.m_type].type;
		auto dt = stdd[dst.m_type].type;
		uint32_t sp = stdd[src.m_type].parts;
		uint32_t dp = stdd[dst.m_type].parts;
		double v[4] = { 0, 0, 0, 0 };
		if (sn)
			v[3] = 1.0;

		for (uint32_t i = 0; i < m_numVertices; ++i) {
			for (uint32_t j = 0; j < sp; ++j)
				switch (st) {
				case STDDT::TF32:
					v[j] = reinterpret_cast<F32 *>(s)[j];
					break;
				case STDDT::TF16:
					v[j] = glm::detail::toFloat32(reinterpret_cast<glm::detail::hdata *>(s)[j]);
					break;
				case STDDT::TU8:
					v[j] = sn ? reinterpret_cast<UINT8 *>(s)[j] / double(0xff) : reinterpret_cast<UINT8 *>(s)[j];
					break;
				case STDDT::TU16:
					v[j] = sn ? reinterpret_cast<UINT16 *>(s)[j] / double(0xffff) : reinterpret_cast<UINT16 *>(s)[j];
					break;
				//case STDDT::TU32:  is used only for indices... so it is not used here
				case STDDT::TI16:
					v[j] = sn ? reinterpret_cast<INT16 *>(s)[j] / double(0x7fff) : reinterpret_cast<INT16 *>(s)[j];
					break;
				case STDDT::TUPACK:
				{
					UINT32 x = s[0];
					v[2] = double(x & 0x3ff) / double(0x3ff);
					x = x >> 10;
					v[1] = double(x & 0x3ff) / double(0x3ff);
					x = x >> 10;
					v[0] = double(x & 0x3ff) / double(0x3ff);
					x = x >> 10;
					v[3] = double(x & 0x3) / double(0x3);
				}
					break;
				case STDDT::TIPACK:
				{
					UINT32 x = s[0], y;
					y = x & 0x3ff;					
					v[2] = y < 0x200 ? double(y) / double(0x1ff) : double(0x400-y) / double(0x1ff);
					x = x >> 10;
					y = x & 0x3ff;
					v[1] = y < 0x200 ? double(y) / double(0x1ff) : double(0x400 - y) / double(0x1ff);
					x = x >> 10;
					y = x & 0x3ff;
					v[0] = y < 0x200 ? double(y) / double(0x1ff) : double(0x400 - y) / double(0x1ff);
					x = x >> 10;
					y = x & 0x3;
					v[3] = y < 0x2 ? double(y) : double(0x4 - y);
				}
					break;
				}

			for (unsigned int j = 0; j < dp; ++j)
				switch (dt) {
				case STDDT::TF32:
					reinterpret_cast<F32 *>(d)[j] = F32(v[j]);
					break;
				case STDDT::TF16:
					reinterpret_cast<I16 *>(d)[j] = glm::detail::toFloat16(F32(v[j]));
					break;

				case STDDT::TU8:
					assert(!dn || (v[j] <= 1.0 && v[j] >= 0.0));
					reinterpret_cast<UINT8 *>(d)[j] = UINT8(dn ? v[j] * double(0xff) : v[j]);
					break;

				case STDDT::TU16:
					assert(!dn || (v[j] <= 1.0 && v[j] >= 0.0));
					reinterpret_cast<UINT16 *>(d)[j] = UINT16(dn ? v[j] * double(0xffff) : v[j]);
					break;
				case STDDT::TI16:
					assert(!dn || (v[j] <= 1.0 && v[j] >= -1.0));
					reinterpret_cast<INT16 *>(d)[j] = INT16(dn ? v[j] * double(0x7fff) : v[j]);
					break;
				case STDDT::TUPACK:
					reinterpret_cast<UINT32 *>(d)[0] = UINT32((INT32(v[2] * 1023) & 0x3ff) << 0 | (INT32(v[1] * 1023) & 0x3ff) << 10 | (INT32(v[0] * 1023) & 0x3ff) << 20 | (INT32(v[3] * 3) & 0x3) << 30);
					break;
				case STDDT::TIPACK:
					reinterpret_cast<UINT32 *>(d)[0] = UINT32((INT32(v[2] * 511) & 0x3ff) << 0 | (INT32(v[1] * 511) & 0x3ff) << 10 | (INT32(v[0] * 511) & 0x3ff) << 20 | (INT32(v[3] * 1) & 0x1) << 30);
					break;
				}

			d += ds;
			s += ss;
		}
	}
	
}

void VertexDescription::Copy(void *&buf, void *&i16, void *&i32, const VertexDescription &src)
{
	auto stride = GetStride();
	auto pv = reinterpret_cast<uint8_t*>(buf);
	ForEachStream([&](Stream &s) {
		s.m_stride = stride;
		s.m_data = pv;
		pv += StreamLen(s);
	});

	m_indices.m_data = m_indices.m_type == VAT_IDX_UINT32_1D ? i32 : i16;

	if (buf && m_indices.m_data) // if we pass buf = 0, we will get size of buf, but not data will be copied
	{
		Copy(src);
	}

	buf = reinterpret_cast<uint8_t*>(buf) + stride * src.m_numVertices;
	if (m_indices.m_type == VAT_IDX_UINT32_1D)
		i32 = reinterpret_cast<uint32_t*>(i32) + src.m_numIndices;
	else
		i16 = reinterpret_cast<uint16_t*>(i16) + src.m_numIndices;
}

void VertexDescription::Copy(const VertexDescription &src)
{
	m_numVertices = src.m_numVertices;
	m_numIndices = src.m_numIndices;

	bool isSingleVertexBlockCompatible = true;
	ptrdiff_t diff = 0;
	ptrdiff_t ldiff = 0, rdiff = 0;
	auto stride = GetStride();
	ForEachStream(src, [&](Stream &d, const Stream &s) {
		if (d.m_type != s.m_type)
		{
			isSingleVertexBlockCompatible = false;
			return;
		}

		if (!isSingleVertexBlockCompatible || d.m_type == VAT_UNUSED)
		{
			return;
		}

		// calc diff
		auto dv = reinterpret_cast<uint8_t*>(m_vertices.m_data);
		auto dp = reinterpret_cast<uint8_t*>(d.m_data);
		auto ld = dp - dv;
		auto rd = ld + StreamLen(d);
		if (ld < ldiff) ldiff = ld;
		if (rd > rdiff) rdiff = rd;
		auto sv = reinterpret_cast<uint8_t*>(src.m_vertices.m_data);
		auto sp = reinterpret_cast<uint8_t*>(s.m_data);
		if (ld != (sp - sv) || stride < (rdiff - ldiff))
		{
			isSingleVertexBlockCompatible = false;
		}
	});

	if (rdiff - ldiff != stride)
		isSingleVertexBlockCompatible = false; // we don't want empty spaces....

	if (isSingleVertexBlockCompatible)
	{
		// all data is in one chunk of memory... just copy mem
		size_t s = m_numVertices * stride;
		memcpy_s(reinterpret_cast<uint8_t*>(m_vertices.m_data) - ldiff, s, reinterpret_cast<uint8_t*>(src.m_vertices.m_data) - ldiff, s);
	}
	else {
		ForEachStream(src, [&](Stream &d, const Stream &s) {
			Copy(d, s);
		});
	}

	if (m_indices.m_type == src.m_indices.m_type && m_indices.m_stride == src.m_indices.m_stride)
	{
		size_t s = (m_indices.m_type == VAT_IDX_UINT32_1D ? sizeof(uint32_t) : sizeof(uint16_t)) * m_numIndices;
		memcpy_s(m_indices.m_data, s, src.m_indices.m_data, s);
	}
	else
	{
		Copy(m_indices, src.m_indices);
	}
}

Stream *VertexDescription::GetStream(uint32_t type)
{
	Stream *s = nullptr;
	switch (type) {
	case VA_POSITION:    s = &m_vertices; break;

	case VA_NORMAL:      s = &m_normals; break;
	case VA_TANGENT:     s = &m_tangents; break;
	case VA_BITANGENT:   s = &m_bitangents; break;

	case VA_COLOR:       s = &m_colors[0]; break;
	case VA_COLOR2:      s = &m_colors[1]; break;
	case VA_COLOR3:      s = &m_colors[2]; break;
	case VA_COLOR4:      s = &m_colors[3]; break;

	case VA_TEXCOORD:    s = &m_textureCoords[0]; break;
	case VA_TEXCOORD2:   s = &m_textureCoords[1]; break;
	case VA_TEXCOORD3:   s = &m_textureCoords[2]; break;
	case VA_TEXCOORD4:   s = &m_textureCoords[3]; break;

	case VA_BONEWEIGHT:  s = &m_boneWeights[0]; break;
	case VA_BONEWEIGHT2: s = &m_boneWeights[1]; break;
	case VA_BONEID:      s = &m_boneIDs[0]; break;
	case VA_BONEID2:     s = &m_boneIDs[1]; break;
	}

	return s;
}

void VertexDescription::Dump(uint32_t numVert, uint32_t numIndices)
{
	static const char *asTxt[] = {
		"VA_POSITION",
		"VA_NORMAL",
		"VA_TANGENT",
		"VA_BITANGENT",
		"VA_TEXCOORD",
		"VA_TEXCOORD2",
		"VA_TEXCOORD3",
		"VA_TEXCOORD4",
		"VA_COLOR",
		"VA_COLOR2",
		"VA_COLOR3",
		"VA_COLOR4",
		"VA_BONEWEIGHT",
		"VA_BONEWEIGHT2",
		"VA_BONEID",
		"VA_BONEID2",
		"VA_INDICES" };

	TRACE("V: " << m_numVertices << ", I: " << m_numIndices << "\n");
	uint32_t t = VA_POSITION;
	auto pr = [&](Stream &s) {
		++t;
		if (s.m_type == VAT_UNUSED)
			return;
		int st = s.m_stride;
		const char *nt = (s.m_normalized ? "true" : "false");
		int len = s.GetDesc().size;
		TRACE("" << asTxt[t - 1] << ": stride: " << st << ", normalized: " << nt << ", size: " << len << "\n");
		for (uint32_t i = 0; i < numVert; ++i)
		{
			TRACE("  " << i << ": ");
			switch (s.m_type) {
			case VAT_FLOAT_1D:
			case VAT_FLOAT_2D:
			case VAT_FLOAT_3D:
			case VAT_FLOAT_4D:
			{
				auto d = reinterpret_cast<float *>(reinterpret_cast<uint8_t *>(s.m_data) + s.m_stride*i);
				for (uint32_t j = 0; j < s.GetDesc().count; ++j)
				{
					if (j != 0) TRACE(", ");
					TRACE(d[j]);
				}
			}
			break;

			case VAT_IDX_UINT16_1D:
			{
				auto d = reinterpret_cast<uint16_t *>(s.m_data) + 3*i;
				TRACE(d[0] << ", " << d[1] << ", " << d[2]);
			}
			break;

			case VAT_IDX_UINT32_1D:
			{
				auto d = reinterpret_cast<uint32_t *>(s.m_data) + 3 * i;
				TRACE(d[0] << ", " << d[1] << ", " << d[2]);
			}
			break;

			default:
			{
				auto d = reinterpret_cast<uint8_t *>(s.m_data) + s.m_stride*i;
				for (uint32_t j = 0; j < s.GetDesc().size; ++j)
				{
					static char hex[] = "0123456789abcdef";
					auto x = d[j];
					if (j != 0 && (j % 4) == 0)
						TRACE(" ");
					TRACE(hex[x & 0xf] << hex[x >> 4]);
				}
			}
			}
			TRACE("\n");
		}
	};
	ForEachStream(pr);
	pr(m_indices);
}

}; // BAMS namespace
