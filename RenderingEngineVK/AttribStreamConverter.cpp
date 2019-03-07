#include "stdafx.h"

using namespace BAMS;
using namespace BAMS::RENDERINENGINE;

struct cv {
	VkFormat vkFormat;
	Stream stream;
	size_t dataLen;
};

struct ConvVertexData {
	U16 len;
	U16 size;
	U16 type;
	bool normalized;

	enum {
		UNKNOWN = 0,
		FLOAT32,
		FLOAT16,
		UINT8,
		INT8,
		UINT16,
		INT16,
		UINT32,
		PACK32,
		UPACK32
	};

}  cvd[] = {
	{ 0, 0, ConvVertexData::UNKNOWN, false },  // UNUSED = 0,
	{ 1, 4, ConvVertexData::FLOAT32, false },  // FLOAT_1D = 1,
	{ 2, 8, ConvVertexData::FLOAT32, false },  // FLOAT_2D = 2,
	{ 3, 12, ConvVertexData::FLOAT32, false }, // FLOAT_3D = 3,
	{ 4, 16, ConvVertexData::FLOAT32, false }, // FLOAT_4D = 4,
	{ 4, 4, ConvVertexData::UINT8, true },  // COL_UINT8_4D = 5, // colors
	{ 1, 2, ConvVertexData::UINT16, false },  // IDX_UINT16_1D = 6, // index buffer
	{ 1, 4, ConvVertexData::UINT32, false },  // IDX_UINT32_1D = 7, // index buffer
	{ 1, 2, ConvVertexData::FLOAT16, false },  // HALF_1D = 8,
	{ 2, 4, ConvVertexData::FLOAT16, false },  // HALF_2D = 9,
	{ 3, 6, ConvVertexData::FLOAT16, false },  // HALF_3D = 10,
	{ 4, 8, ConvVertexData::FLOAT16, false },  // HALF_4D = 11,

	{ 1, 2, ConvVertexData::INT16, true },  // N_SHORT_1D = 12,
	{ 2, 4, ConvVertexData::INT16, true },  // N_SHORT_2D = 13,
	{ 3, 6, ConvVertexData::INT16, true },  // N_SHORT_3D = 14,
	{ 4, 8, ConvVertexData::INT16, true },  // N_SHORT_4D = 15, // not needed

	{ 1, 2, ConvVertexData::UINT16, true },  // N_USHORT_1D = 16,
	{ 2, 4, ConvVertexData::UINT16, true },  // N_USHORT_2D = 17,
	{ 3, 6, ConvVertexData::UINT16, true },  // N_USHORT_3D = 18,
	{ 4, 8, ConvVertexData::UINT16, true },  // N_USHORT_4D = 19, // not needed

	{ 1, 4, ConvVertexData::PACK32, true },  // INT_PACKED = 20,
	{ 1, 4, ConvVertexData::UPACK32, true },  // UINT_PACKED = 21,

	{ 1, 2, ConvVertexData::UINT16, false },  // UINT16_1D = 22,
	{ 2, 4, ConvVertexData::UINT16, false },  // UINT16_2D = 23,
	{ 3, 6, ConvVertexData::UINT16, false },  // UINT16_3D = 24,
	{ 4, 8, ConvVertexData::UINT16, false },  // UINT16_4D = 25,

	{ 1, 1, ConvVertexData::UINT8, false },  // UINT8_1D = 26,
	{ 2, 2, ConvVertexData::UINT8, false },  // UINT8_2D = 27,
	{ 3, 3, ConvVertexData::UINT8, false },  // UINT8_3D = 28,
	{ 4, 4, ConvVertexData::UINT8, false },  // UINT8_4D = 29,
};

cv conv[] = {
	{ VK_FORMAT_R32_SFLOAT,          Stream((U32)FLOAT_1D, 1 * sizeof(F32), false), 1 * sizeof(F32) },
	{ VK_FORMAT_R32G32_SFLOAT,       Stream((U32)FLOAT_2D, 2 * sizeof(F32), false), 2 * sizeof(F32) },
	{ VK_FORMAT_R32G32B32_SFLOAT,    Stream((U32)FLOAT_3D, 3 * sizeof(F32), false), 3 * sizeof(F32) },
	{ VK_FORMAT_R32G32B32A32_SFLOAT, Stream((U32)FLOAT_4D, 4 * sizeof(F32), false), 4 * sizeof(F32) },

	{ VK_FORMAT_R8_UNORM,            Stream((U32)COL_UINT8_4D, 1 * sizeof(U8), true), 1 * sizeof(U32) },
	{ VK_FORMAT_R8G8_UNORM,          Stream((U32)COL_UINT8_4D, 2 * sizeof(U8), true), 1 * sizeof(U32) },
	{ VK_FORMAT_R8G8B8_UNORM,        Stream((U32)COL_UINT8_4D, 3 * sizeof(U8), true), 1 * sizeof(U32) },
	{ VK_FORMAT_R8G8B8A8_UNORM,      Stream((U32)COL_UINT8_4D, 4 * sizeof(U8), true), 1 * sizeof(U32) },
	{ VK_FORMAT_R16_UINT,            Stream((U32)IDX_UINT16_1D, 1 * sizeof(U16), false), 1 * sizeof(U16) },
	{ VK_FORMAT_R32_UINT,            Stream((U32)IDX_UINT32_1D, 1 * sizeof(U32), false), 1 * sizeof(U32) },

	{ VK_FORMAT_R16_SFLOAT,          Stream((U32)HALF_1D, 2 * sizeof(U16), false), 1 * sizeof(U16) },
	{ VK_FORMAT_R16G16_SFLOAT,       Stream((U32)HALF_2D, 2 * sizeof(U16), false), 2 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16_SFLOAT,    Stream((U32)HALF_3D, 4 * sizeof(U16), false), 3 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16A16_SFLOAT, Stream((U32)HALF_4D, 4 * sizeof(U16), false), 4 * sizeof(U16) },

	{ VK_FORMAT_R16_SNORM,           Stream((U32)N_SHORT_1D, 2 * sizeof(I16), true), 1 * sizeof(U16) },
	{ VK_FORMAT_R16G16_SNORM,        Stream((U32)N_SHORT_2D, 2 * sizeof(I16), true), 2 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16_SNORM,     Stream((U32)N_SHORT_3D, 4 * sizeof(I16), true), 3 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16A16_SNORM,  Stream((U32)N_SHORT_4D, 4 * sizeof(I16), true), 4 * sizeof(U16) },

	{ VK_FORMAT_R16_UNORM,           Stream((U32)N_USHORT_1D, 2 * sizeof(U16), true), 1 * sizeof(U16) },
	{ VK_FORMAT_R16G16_UNORM,        Stream((U32)N_USHORT_2D, 2 * sizeof(U16), true), 2 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16_UNORM,     Stream((U32)N_USHORT_3D, 4 * sizeof(U16), true), 3 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16A16_UNORM,  Stream((U32)N_USHORT_4D, 4 * sizeof(U16), true), 4 * sizeof(U16) },

	{ VK_FORMAT_A2R10G10B10_UNORM_PACK32, Stream((U32)UINT_PACKED, 1 * sizeof(U32), true), 1 * sizeof(U32) },
	{ VK_FORMAT_A2R10G10B10_SNORM_PACK32, Stream((U32)INT_PACKED,  1 * sizeof(U32), true), 1 * sizeof(U32) },

	{ VK_FORMAT_R16_UINT,            Stream((U32)UINT16_1D, 2 * sizeof(U16), false), 1 * sizeof(U16) },
	{ VK_FORMAT_R16G16_UINT,         Stream((U32)UINT16_2D, 2 * sizeof(U16), false), 2 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16_UINT,      Stream((U32)UINT16_3D, 4 * sizeof(U16), false), 3 * sizeof(U16) },
	{ VK_FORMAT_R16G16B16A16_UINT,   Stream((U32)UINT16_4D, 4 * sizeof(U16), false), 4 * sizeof(U16) },

	{ VK_FORMAT_R8_UINT,             Stream((U32)UINT8_1D, 2 * sizeof(U16), false), 1 * sizeof(U8) },
	{ VK_FORMAT_R8G8_UINT,           Stream((U32)UINT8_2D, 2 * sizeof(U16), false), 2 * sizeof(U8) },
	{ VK_FORMAT_R8G8B8_UINT,         Stream((U32)UINT8_3D, 4 * sizeof(U16), false), 3 * sizeof(U8) },
	{ VK_FORMAT_R8G8B8A8_UINT,       Stream((U32)UINT8_4D, 4 * sizeof(U16), false), 4 * sizeof(U8) },
};


void CAttribStreamConverter::Convert(VkStream vkdst, VkStream vksrc, uint32_t len)
{
	Stream dst;
	Stream src;
	ConvertAttribStreamDescription(dst, vkdst);
	ConvertAttribStreamDescription(src, vksrc);
	Convert(dst, src, len);
}

void CAttribStreamConverter::Convert(Stream &dst, const Stream &src, uint32_t len)
{
	auto s = static_cast<U8*>(src.m_data);
	auto d = static_cast<U8*>(dst.m_data);
	auto &c = cvd[src.m_type];
	auto &c2 = cvd[dst.m_type];
	auto l = c.size;
	auto sstride = src.m_stride;
	auto dstride = dst.m_stride;

	if (dst.m_type == src.m_type)
	{
		// just copy data
		if (l == sstride && l == dstride) 
		{
			memcpy_s(d, l*len, s, l*len);
			return;
		}
		while (len) 
		{
			memcpy_s(d, l, s, l);
			s += sstride;
			d += dstride;
			--len;
		}
		return;
	}

	float v[4] = { 0.0f, 0.0f, 0.0f, c.normalized ? 1.0f : 0.0f };
	U32 x;
	I32 y;

	for (uint32_t i = 0; i < len; ++i)
	{
		// src to V
		switch (c.type) {
		case ConvVertexData::FLOAT32:
			for (unsigned int j = 0; j < c.len; ++j)
				v[j] = reinterpret_cast<F32 *>(s)[j];
			break;

		case ConvVertexData::UINT8:
			if (c.normalized) {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<U8 *>(s)[j] / float(0xff);

			}
			else {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<U8 *>(s)[j];
			}
			break;

		case ConvVertexData::UINT16:
			if (c.normalized) {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<U16 *>(s)[j] / float(0xffff);

			}
			else {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<U16 *>(s)[j];
			}
			break;

		case ConvVertexData::UINT32:
			if (c.normalized) {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<U32*>(s)[j] / float(0xffffffff);

			}
			else {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = static_cast<F32>(reinterpret_cast<U32 *>(s)[j]);
			}
			break;

		case ConvVertexData::INT8:
			if (c.normalized) {
				for (unsigned int j = 0; j < c.len; ++j)
				{
					auto t = reinterpret_cast<I8 *>(s)[j];
					v[j] = t == -128 ? -1 : t / float(0x7f);
				}

			}
			else {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<I8 *>(s)[j];
			}
			break;

		case ConvVertexData::INT16:
			if (c.normalized) {
				for (unsigned int j = 0; j < c.len; ++j)
				{
					auto t = reinterpret_cast<I16 *>(s)[j];
					v[j] = t == -32768 ? -1 : t / float(0x7fff);
				}

			}
			else {
				for (unsigned int j = 0; j < c.len; ++j)
					v[j] = reinterpret_cast<I16 *>(s)[j];
			}
			break;


		case ConvVertexData::FLOAT16:
			for (unsigned int j = 0; j < c.len; ++j)
				v[j] = Utils::Float16Compressor::decompress(reinterpret_cast<uint16_t *>(s)[j]);

		case ConvVertexData::PACK32:
			y = reinterpret_cast<I32 *>(s)[0];
			v[0] = ((y & 0x200 ) ? (-512 + (y & 0x1ff)): (y & 0x1ff)) / float(0x1ff);
			v[1] = ((y >> 10) & 0x200 ? -512 + ((y >> 10) & 0x1ff) : ((y >> 10) & 0x1ff)) / float(0x1ff);
			v[2] = ((y >> 20) & 0x200 ? -512 + ((y >> 20) & 0x1ff) : ((y >> 20) & 0x1ff)) / float(0x1ff);
			v[3] = float((y >> 30) & 0x2   ? -2   + ((y >> 30) & 0x1) : ((y >> 30) & 0x1));
			break;

		case ConvVertexData::UPACK32:
			x = reinterpret_cast<U32 *>(s)[0];
			v[0] = (x & 0x3ff) / float(0x3ff);
			v[1] = ((x >> 10) & 0x3ff) / float(0x3ff);
			v[2] = ((x >> 20) & 0x3ff) / float(0x3ff);
			v[3] = ((x >> 30) & 0x3) / float(0x3);
			break;
		}

		// ========================================== V to dst ===================================
		switch (c2.type) {
		case ConvVertexData::FLOAT32:
			for (unsigned int j = 0; j < c2.len; ++j)
				reinterpret_cast<F32 *>(d)[j] = v[j];
			break;

		case ConvVertexData::UINT8:
			if (c.normalized) {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U8 *>(d)[j] = static_cast<U8>(v[j] * float(0xff));

			}
			else {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U8 *>(d)[j] = static_cast<U8>(v[j]);
			}
			break;

		case ConvVertexData::UINT16:
			if (c.normalized) {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U16 *>(d)[j] = static_cast<U16>(v[j] * float(0xffff));

			}
			else {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U16 *>(d)[j] = static_cast<U16>(v[j]);
			}
			break;

		case ConvVertexData::UINT32:
			if (c.normalized) {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U32*>(d)[j] = static_cast<U32>(v[j] * float(0xffffffff));

			}
			else {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<U32*>(d)[j] = static_cast<U32>(v[j]);
			}
			break;

		case ConvVertexData::INT8:
			if (c.normalized) {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<I8 *>(d)[j] = static_cast<I8>(v[j] * float(0x7f));

			}
			else {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<I8 *>(d)[j] = static_cast<I8>(v[j]);
			}
			break;

		case ConvVertexData::INT16:
			if (c.normalized) {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<I16 *>(d)[j] = static_cast<I16>(v[j] * float(0x7fff));
			}
			else {
				for (unsigned int j = 0; j < c2.len; ++j)
					reinterpret_cast<I16 *>(d)[j] = static_cast<I16>(v[j]);
			}
			break;


		case ConvVertexData::FLOAT16:
			for (unsigned int j = 0; j < c2.len; ++j)
				reinterpret_cast<uint16_t *>(d)[j] = Utils::Float16Compressor::compress(v[j]);

		case ConvVertexData::PACK32:
			reinterpret_cast<U32 *>(d)[0] = U32((I32(v[0] * 0x1ff) & 0x3ff) << 0 | (I32(v[1] * 0x1ff) & 0x3ff) << 10 | (I32(v[2] * 0x1ff) & 0x3ff) << 20 | (I32(v[3] * 1) & 0x3) << 30);
			break;

		case ConvVertexData::UPACK32:
			reinterpret_cast<U32 *>(d)[0] = U32((U32(v[0] * 0x3ff) & 0x3ff) << 0 | (U32(v[1] * 0x3ff) & 0x3ff) << 10 | (U32(v[2] * 0x3ff) & 0x3ff) << 20 | (U32(v[3] * 3) & 0x3) << 30);
			break;
		}

		s += sstride;
		d += dstride;		
	}
}


void CAttribStreamConverter::ConvertAttribStreamDescription(Stream &out, VkStream src)
{
	for (auto &s : conv)
	{
		if (s.vkFormat == src.format)
		{
			out = s.stream;
			out.m_stride = src.stride;
			out.m_data = src.data;
			break;
		}
	}
}


void CAttribStreamConverter::ConvertAttribStreamDescription(VkStream &out, Stream src)
{
	for (auto &s : conv)
	{
		if (s.stream.m_normalized == src.m_normalized &&
			s.stream.m_type == src.m_type)
		{
			out.format = s.vkFormat;
			out.stride = src.m_stride;
			out.data = src.m_data;
			break;
		}
	}
}