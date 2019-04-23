#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"

using namespace spirv_cross;
using namespace BAMS::RENDERINENGINE;
const char *SharedUniformBufferObject = "UniformBufferObject";

/*
enum {
	UNKNOWN = -1,
	VERTEX = 0,
	NORMAL,
	TANGENT,
	BITANGENT,
	TEXCOORD,
	TEXCOORD2,
	TEXCOORD3,
	TEXCOORD4,
	COLOR,
	COLOR2,
	COLOR3,
	COLOR4,
	BONEWEIGHT,
	BONEWEIGHT2,
	BONEID,
	BONEID2
};
*/

uint32_t FindVertexAttribType(const std::string &name)
{
	static std::vector<std::vector<std::string>> VertexAttribAliases = {
	{ "Vertex", "Position", "inPosition", "Vert", "inPos" },
	{ "Normal", "inNormal", "Norm", "inNor" },
	{ "Tangent", "inTangent", "Tang", "inTan" },
	{ "Bitangent", "inBitangent" "Bitang", "inBit" },

	{ "Texture",  "Texture1", "inTexture", "inTexture1", "Tex", "Tex1", "inUV", "inUV1" },
	{ "Texture2", "inTexture2", "Tex2", "inUV2" },
	{ "Texture3", "inTexture3", "Tex3", "inUV3" },
	{ "Texture4", "inTexture4", "Tex4", "inUV3" },

	{ "Color", "Color1", "inColor", "inColor1", "Col", "Col1" },
	{ "Color2", "inColor2", "Col2" },
	{ "Color3", "inColor2", "Col3" },
	{ "Color4", "inColor2", "Col4" },

	{ "BoneWeight", "BWeight", "BoneWeight1", "BWeight1", },
	{ "BoneWeight2", "BWeight2", },
	{ "BoneId", "BId", "BondId1", "BId1" },
	{ "BondId2", "BId2" },
	};

	for (uint32_t i = VA_POSITION; i <= VA_BONEID2; ++i) {
		for (auto &alias : VertexAttribAliases[i]) {
			if (Utils::icasecmp(name, alias)) {
				return i;
			}
		}
	}

	return VA_UNKNOWN;
}

void SetVkFormat(VertexAttribDesc &vad)
{
	static VkFormat f32_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT };
	static VkFormat f16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
	static VkFormat i16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT };
	static VkFormat u16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT };
	static VkFormat i8_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SINT, VK_FORMAT_R8G8_SINT, VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT };
	static VkFormat u8_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT };
	static VkFormat p10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32 };
	static VkFormat up10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32 };
	static VkFormat u8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
	static VkFormat i8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SNORM, VK_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM };
	static VkFormat i16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM };
	static VkFormat u16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM };

	static uint32_t f32_size[5] = { 0, 4, 8, 12, 16 };
	static uint32_t f16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t i16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t u16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t i8_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t u8_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t p10_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t up10_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t i8n_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t u8n_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t i16n_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t u16n_size[5] = { 0, 4, 4, 8, 8 };
	struct {
		VkFormat *vkf;
		uint32_t *vkl;
	} vk[] = {
		nullptr, nullptr,         // UNKNOWN
		f32_formats, f32_size,    // FLOAT_1D
		f32_formats, f32_size,    // FLOAT_2D
		f32_formats, f32_size,    // FLOAT_3D
		f32_formats, f32_size,    // FLOAT_4D

		u8n_formats, u8n_size,    // COL_UINT8_4D

		u16_formats, u16_size,    // IDX_UINT16_1D  *** will not occur as attrib
		u16_formats, u16_size,    // IDX_UINT32_1D  *** will not occur as attrib

		f16_formats, f16_size,    // HALF_1D
		f16_formats, f16_size,    // HALF_2D
		f16_formats, f16_size,    // HALF_3D
		f16_formats, f16_size,    // HALF_4D

		i16n_formats, i16n_size,  // N_SHORT_1D
		i16n_formats, i16n_size,  // N_SHORT_2D
		i16n_formats, i16n_size,  // N_SHORT_3D
		i16n_formats, i16n_size,  // N_SHORT_4D

		u16n_formats, u16n_size,  // N_USHORT_1D
		u16n_formats, u16n_size,  // N_USHORT_2D
		u16n_formats, u16n_size,  // N_USHORT_3D
		u16n_formats, u16n_size,  // N_USHORT_4D

		p10_formats, p10_size,    // INT_PACKED
		up10_formats, up10_size,  // UINT_PACKED

		u16_formats, u16_size,    // UINT16_1D
		u16_formats, u16_size,    // UINT16_2D
		u16_formats, u16_size,    // UINT16_3D
		u16_formats, u16_size,    // UINT16_4D

		u8_formats, u8_size,      // UINT8_1D
		u8_formats, u8_size,      // UINT8_2D
		u8_formats, u8_size,      // UINT8_3D
		u8_formats, u8_size,      // UINT8_4D
	};

	auto &t = vk[vad.pStream->m_type];
	vad.format = t.vkf[vad.vecsize];
	vad.size = t.vkl[vad.vecsize];
}

void FindVertexAttribFormatAndSize(VertexAttribDesc &vd)
{
	// note: on AMD RX 580 it crash in amdvlk64.dll ver 25.20.15031.1000 with div by zero error when  VK_FORMAT_R8G8B8_?? and VK_FORMAT_R16G16B16_?? is used
	static VkFormat f32_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT };
	static VkFormat f16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
	static VkFormat i16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16A16_SINT, VK_FORMAT_R16G16B16A16_SINT };
	static VkFormat u16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16A16_UINT, VK_FORMAT_R16G16B16A16_UINT };
	static VkFormat i8_formats[5]  = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SINT, VK_FORMAT_R8G8_SINT, VK_FORMAT_R8G8B8A8_SINT, VK_FORMAT_R8G8B8A8_SINT };
	static VkFormat u8_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8B8A8_UINT, VK_FORMAT_R8G8B8A8_UINT };
	static VkFormat p10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_UNDEFINED };
	static VkFormat up10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_UNDEFINED };
	static VkFormat u8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8A8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
	static VkFormat i8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SNORM, VK_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8B8A8_SNORM, VK_FORMAT_R8G8B8A8_SNORM };
	static VkFormat i16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16B16A16_SNORM, VK_FORMAT_R16G16B16A16_SNORM };
	static VkFormat u16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16B16A16_UNORM, VK_FORMAT_R16G16B16A16_UNORM }; 
	static uint32_t f32_size[5] = { 0, 4, 8, 12, 16 };
	static uint32_t f16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t i16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t u16_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t i8_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t u8_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t p10_size[5] = { 0, 0, 4, 4, 0 };
	static uint32_t up10_size[5] = { 0, 0, 4, 4, 0 };
	static uint32_t i8n_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t u8n_size[5] = { 0, 4, 4, 4, 4 };
	static uint32_t i16n_size[5] = { 0, 4, 4, 8, 8 };
	static uint32_t u16n_size[5] = { 0, 4, 4, 8, 8 };


	vd.format = VK_FORMAT_UNDEFINED;
	vd.size = 0;
	if (vd.vecsize < 1 || vd.vecsize > 4)
		return;

	auto &o = *GetOptimize();
	uint32_t op = NONE;
	switch (vd.type) {
	case VA_POSITION: // vecsize = 1 and vecsize = 4 are pointless.... but maybe not?
		break;

	case VA_NORMAL:
	case VA_TANGENT:
	case VA_BITANGENT:
		op = o.normals;
		break;

	case VA_COLOR:
	case VA_COLOR2:
	case VA_COLOR3:
	case VA_COLOR4:
		op = o.colors;
		break;

	case VA_TEXCOORD:
	case VA_TEXCOORD2:
	case VA_TEXCOORD3:
	case VA_TEXCOORD4:
		op = o.coords;
		break;

	case VA_BONEWEIGHT:
	case VA_BONEWEIGHT2:
		op = o.bone_weights;
		break;

	case VA_BONEID:
	case VA_BONEID2:
		op = o.bone_ids;
		break;
	}


	switch (op) {
	case FLOATS_TO_INT16: vd.format = i16n_formats[vd.vecsize]; vd.size = i16_size[vd.vecsize]; break;
	case FLOATS_TO_UINT16: vd.format = u16n_formats[vd.vecsize]; vd.size = u16_size[vd.vecsize]; break;
	case FLOATS_TO_INT8: vd.format = i8n_formats[vd.vecsize]; vd.size = i8_size[vd.vecsize]; break;
	case FLOATS_TO_UINT8: vd.format = u8n_formats[vd.vecsize];  vd.size = u8_size[vd.vecsize]; break;
	case FLOATS_TO_PACKED_INT10: vd.format = p10_formats[vd.vecsize]; vd.size = p10_size[vd.vecsize]; break;
	case FLOATS_TO_PACKED_UINT10: vd.format = up10_formats[vd.vecsize]; vd.size = up10_size[vd.vecsize]; break;
	case UINT16_TO_UINT8: vd.format = u8_formats[vd.vecsize];  vd.size = u8_size[vd.vecsize]; break;
	default:
		vd.format = f32_formats[vd.vecsize]; vd.size = f32_size[vd.vecsize];
	}
}

void SetSimplifiedType(ValMemberDetails &mem)
{
	if (mem.type == ShaderReflectionType::Float32)
	{
		if (mem.size == 8 && mem.vecsize == 2)
			mem.type = ShaderReflectionType::Vec2;
		else if (mem.size == 12 && mem.vecsize == 3)
			mem.type = ShaderReflectionType::Vec3;
		else if (mem.size == 16 && mem.vecsize == 4)
			mem.type = ShaderReflectionType::Vec4;
		else if (mem.size == 36 && mem.vecsize == 3 && mem.colsize == 3)
			mem.type = ShaderReflectionType::Mat3;
		else if (mem.size == 64 && mem.vecsize == 4 && mem.colsize == 4)
			mem.type = ShaderReflectionType::Mat4;
	}
}

void GetDetails(CompilerGLSL &comp, SPIRType &type, ValMemberDetails &ent)
{
	static ShaderReflectionType typeConv[] = {
			ShaderReflectionType::UNKNOWN, // BaseType::Unknown
			ShaderReflectionType::UNKNOWN, // BaseType::Void
			ShaderReflectionType::UNKNOWN, // BaseType::Boolean
			ShaderReflectionType::UNKNOWN, // BaseType::Char
			ShaderReflectionType::Int32,   // BaseType::Int

			ShaderReflectionType::UInt32,  // BaseType::UInt
			ShaderReflectionType::UNKNOWN, // BaseType::Int64
			ShaderReflectionType::UNKNOWN, // BaseType::UInt64
			ShaderReflectionType::UNKNOWN, // BaseType::AtomicCounter
			ShaderReflectionType::UNKNOWN, // BaseType::Half

			ShaderReflectionType::Float32, // BaseType::Float
			ShaderReflectionType::UNKNOWN, // BaseType::Double
			ShaderReflectionType::UNKNOWN, // BaseType::Struct (UBOs too)
			ShaderReflectionType::UNKNOWN, // BaseType::Image
			ShaderReflectionType::UNKNOWN, // BaseType::SampledImage

			ShaderReflectionType::UNKNOWN, // BaseType::Sampler
	};

	static uint32_t typeSize[] = {
		0, 0, 0, 1, 4,
		4, 8, 8, 0, 2,
		4, 8, 0, 0, 0,
		0
	};

	ent.array = type.array.size() == 1 ? type.array[0] : 1;
	ent.vecsize = type.vecsize;
	ent.colsize = type.columns;

	if (type.basetype == SPIRType::BaseType::Struct && type.member_types.size())
	{
		//ent.size = static_cast<uint32_t>(comp.get_declared_struct_size(comp.get_type(res.base_type_id)));
		//ent.size = static_cast<uint32_t>(comp.get_declared_struct_size(comp.get_type(type.parent_type)));
		auto &members = type.member_types;
		for (uint32_t i = 0; i < members.size(); ++i)
		{
			SPIRType member_type = comp.get_type(members[i]);
			ValMemberDetails mem;
			mem.name = comp.get_member_name(type.self, i);
			mem.typenName = comp.get_name(members[i]);
			mem.offset = comp.type_struct_member_offset(type, i);
			mem.size = static_cast<uint32_t>(comp.get_declared_struct_member_size(type, i));
			mem.array = !member_type.array.empty() ? member_type.array[0] : 1;
			mem.array_stride = !member_type.array.empty() ? comp.type_struct_member_array_stride(type, i) : 0;
			mem.vecsize = member_type.vecsize;
			mem.colsize = member_type.columns;
			mem.matrix_stride = comp.has_decoration(i, spv::DecorationMatrixStride) ? comp.type_struct_member_matrix_stride(type, i) : 0;
			mem.type = typeConv[member_type.basetype];
			SetSimplifiedType(mem);
			if (member_type.basetype == SPIRType::BaseType::Struct && member_type.member_types.size())
				GetDetails(comp, member_type, mem);

			ent.members.push_back(mem);
		}
	}
	else if (typeConv[type.basetype] != ShaderReflectionType::UNKNOWN)
	{
		ent.type = typeConv[type.basetype];
		ent.size = typeSize[type.basetype] * ent.vecsize * ent.colsize * ent.array;
		SetSimplifiedType(ent);
	}

}

void GetDetails(CompilerGLSL &comp, Resource &res, ValDetails &det)
{
	det = {};
	det.set = comp.has_decoration(res.id, spv::DecorationDescriptorSet) ? det.set = comp.get_decoration(res.id, spv::DecorationDescriptorSet) : 0;
	det.binding = comp.has_decoration(res.id, spv::DecorationBinding) ? comp.get_decoration(res.id, spv::DecorationBinding) : 0;

	det.entry.name = comp.get_name(res.id);
	det.entry.typenName = comp.get_name(res.base_type_id);
	SPIRType type = comp.get_type(res.type_id);
	if (type.basetype == SPIRType::BaseType::Struct && type.member_types.size())
	{
		det.entry.size = static_cast<uint32_t>(comp.get_declared_struct_size(comp.get_type(res.base_type_id)));
	}
	GetDetails(comp, type, det.entry);
}

// ============================================================================ ShadersReflections ===

CShadersReflections::CShadersReflections() {}
CShadersReflections::CShadersReflections(const char *shaderName) { LoadProgram(shaderName); }

/// <summary>
/// Loads shader program.
/// </summary>
/// <param name="shaderName">The shader program name from resource.</param>
ShaderDataInfo CShadersReflections::LoadProgram(const char *shaderName)
{
	BAMS::CResourceManager rm;
	m_shaderResource = rm.GetShaderByName(shaderName);
	if (!m_shaderResource.IsLoaded())
		rm.LoadSync(m_shaderResource);
	uint32_t cnt = m_shaderResource.GetBinaryCount();

	m_programs.resize(cnt);

	for (uint32_t i = 0; i < cnt; ++i)
	{
		auto &prg = m_programs[i];
		prg.entryPointName = "main";
		prg.resource = m_shaderResource.GetBinary(i);
		if (!prg.resource.IsLoaded())
			rm.LoadSync(prg.resource);
	}

	_ParsePrograms();
	return vi;
}

// ============================================================================ ShadersReflections private methods ===

VkFormat GetVkFormat(uint32_t streamFormat)
{
	VkFormat mainFormats[] = {
		VK_FORMAT_R32G32B32A32_SFLOAT,  // VAT_UNUSED = 0,
		VK_FORMAT_R32_SFLOAT,           // VAT_FLOAT_1D = 1,
		VK_FORMAT_R32G32_SFLOAT,        // VAT_FLOAT_2D = 2,
		VK_FORMAT_R32G32B32_SFLOAT,     // VAT_FLOAT_3D = 3,
		VK_FORMAT_R32G32B32A32_SFLOAT,  // VAT_FLOAT_4D = 4,
	    VK_FORMAT_R8G8B8A8_UNORM,       // VAT_COL_UINT8_4D = 5, // colors
		VK_FORMAT_R16_UINT,             // VAT_IDX_UINT16_1D = 6, // index buffer
		VK_FORMAT_R32_UINT,             // VAT_IDX_UINT32_1D = 7, // index buffer
		VK_FORMAT_R16G16_SFLOAT,        // VAT_HALF_1D = 8,
		VK_FORMAT_R16G16_SFLOAT,        // VAT_HALF_2D = 9,
		VK_FORMAT_R16G16B16A16_SFLOAT,  // VAT_HALF_3D = 10,
		VK_FORMAT_R16G16B16A16_SFLOAT,  // VAT_HALF_4D = 11,
		VK_FORMAT_R16G16_SNORM,         // VAT_N_SHORT_1D = 12,  // used for normals/tangents... _N_ = normalized
		VK_FORMAT_R16G16_SNORM,         // VAT_N_SHORT_2D = 13,
		VK_FORMAT_R16G16B16A16_SNORM,   // VAT_N_SHORT_3D = 14,
		VK_FORMAT_R16G16B16A16_SNORM,   // VAT_N_SHORT_4D = 15, // not needed	
	    VK_FORMAT_R16G16_UNORM,         // VAT_N_USHORT_1D = 16,  // used for colors
		VK_FORMAT_R16G16_UNORM,         // VAT_N_USHORT_2D = 17,
		VK_FORMAT_R16G16B16A16_UNORM,   // VAT_N_USHORT_3D = 18,
		VK_FORMAT_R16G16B16A16_UNORM,   // VAT_N_USHORT_4D = 19, // not needed	
		VK_FORMAT_A2R10G10B10_SNORM_PACK32, // VAT_INT_PACKED = 20,  // packed formats: GL_INT_2_10_10_10_REV & GL_UNSIGNED_INT_2_10_10_10_REV
		VK_FORMAT_A2R10G10B10_UNORM_PACK32, // VAT_UINT_PACKED = 21,
		VK_FORMAT_R16_UINT,             // VAT_UINT16_1D = 22,
		VK_FORMAT_R16G16_UINT,          // VAT_UINT16_2D = 23,
		VK_FORMAT_R16G16B16A16_UINT,    // VAT_UINT16_3D = 24,
		VK_FORMAT_R16G16B16A16_UINT,    // VAT_UINT16_4D = 25,
		VK_FORMAT_R8G8B8A8_UINT,        // VAT_UINT8_1D = 26,
		VK_FORMAT_R8G8B8A8_UINT,        // VAT_UINT8_2D = 27,
		VK_FORMAT_R8G8B8A8_UINT,        // VAT_UINT8_3D = 28,
		VK_FORMAT_R8G8B8A8_UINT,        // VAT_UINT8_4D = 29,
	};
	auto f = mainFormats[streamFormat];

	// TODO: Add format check and fallbacks

	return f;
}

VertexAttribDesc GetVertexAttribDecription(CompilerGLSL &compiler, Resource &attrib, VertexDescription &vd)
{
	VertexAttribDesc vad = {
		compiler.get_decoration(attrib.id, spv::DecorationBinding),
		compiler.get_decoration(attrib.id, spv::DecorationLocation),
		compiler.get_type(attrib.type_id).vecsize,
		FindVertexAttribType(compiler.get_name(attrib.id))
	};
	
	vad.pStream = vd.GetStream(vad.type);
	vad.pStream->m_type = GetOptimize()->GetOptimizedType(vad.type, vad.vecsize);
	vad.pStream->m_normalized = vad.pStream->GetDesc().normalized;
	vad.format = GetVkFormat(vad.pStream->m_type);
	vad.size = vad.pStream->GetDesc().size;
	return std::move(vad);
}

/// <summary>
/// Parses shader programs (with reflection from SPIRV-Cross).
/// We get:
/// 1. Vertex attributes definition.
/// 2. UBOs description (with names of variables, size, type, ...)
/// 3. Names of output framebuffers (they are used to select render pass)
/// </summary>
void CShadersReflections::_ParsePrograms()
{
	m_layout = {}; // clear
	static VkShaderStageFlagBits executionModelToStage[] = {
		VK_SHADER_STAGE_VERTEX_BIT,
		VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
		VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
		VK_SHADER_STAGE_GEOMETRY_BIT,
		VK_SHADER_STAGE_FRAGMENT_BIT,
		VK_SHADER_STAGE_COMPUTE_BIT
	};

	m_enableAlpha = false;
	vi.params_in_ubos.clear();
	vi.params_in_push_constants.clear();

	vi.total_ubos_size = 0;
	vi.shared_ubos_size = 0;
	vi.max_single_ubo_size = 0;
	vi.ubo_sizes.clear();

	for (auto &prg : m_programs) 
	{		
		auto code = prg.resource;
		auto bin = (uint32_t*)code.GetData();
		assert(bin != nullptr);
		auto blen = (code.GetSize() + sizeof(uint32_t) - 1) / sizeof(uint32_t);
		CompilerGLSL compiler((uint32_t*)code.GetData(), (code.GetSize() + sizeof(uint32_t) - 1) / sizeof(uint32_t));
		auto resources = compiler.get_shader_resources();
		auto entry_points = compiler.get_entry_points_and_stages();
		auto &entry_point = entry_points[0];
		prg.entryPointName = entry_point.name;
		prg.stage = executionModelToStage[entry_point.execution_model];

		for (auto buffer : resources.uniform_buffers)
		{
			auto set = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
			auto binding = compiler.get_decoration(buffer.id, spv::DecorationBinding);

			ValDetails vd;
			GetDetails(compiler, buffer, vd);
			vd.stage = prg.stage;
			vi.params_in_ubos.push_back(vd);
			if (vd.entry.typenName == SharedUniformBufferObject)
			{
				vi.shared_ubos_size += vd.entry.size;
			}
			else {
				vi.total_ubos_size += vd.entry.size;
				if (vi.max_single_ubo_size < vd.entry.size)
					vi.max_single_ubo_size = vd.entry.size;

				if (vi.ubo_sizes.size() <= vd.binding)
				{
					size_t i = vi.ubo_sizes.size();
					vi.ubo_sizes.resize(vd.binding + 1);
					for (; i < vd.binding; ++i)
						vi.ubo_sizes[i] = 0;
				}
				vi.ubo_sizes[vd.binding] = vd.entry.size;
			}

			auto name = compiler.get_name(buffer.id);
			SPIRType type = compiler.get_type(buffer.type_id);
			uint32_t descriptorCount = type.array.size() == 1 ? type.array[0] : 1;

			m_layout.descriptorSets[set].uniform_buffer_mask |= 1u << binding;
			m_layout.descriptorSets[set].descriptorCount[binding] = descriptorCount;
			m_layout.descriptorSets[set].stages[binding] |= prg.stage;
		}

		for (auto pushConst : resources.push_constant_buffers)
		{
			uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(compiler.get_type(pushConst.base_type_id)));
			ValDetails vd;
			GetDetails(compiler, pushConst, vd);
			vd.stage = prg.stage;
			vi.params_in_push_constants.push_back(vd);
		}

		if (prg.stage == VK_SHADER_STAGE_VERTEX_BIT)
		{
			if (resources.stage_inputs.size() == 4)
				TRACE("NOW");
			vi.attribs.clear();
			vi.descriptions = {}; // clear
			for (auto attrib : resources.stage_inputs) 
			{
				VertexAttribDesc vad = GetVertexAttribDecription(compiler, attrib, vi.descriptions);
				vi.attribs.push_back(vad);				
			}
		}

		if (prg.stage == VK_SHADER_STAGE_FRAGMENT_BIT) 
		{
			m_outputNames.clear();
			for (uint32_t i = 0; i < resources.stage_outputs.size(); ++i)
			{
				m_outputNames.push_back(compiler.get_name(resources.stage_outputs[0].id));
			}
			m_enableAlpha = resources.stage_outputs.size() == 1 && Utils::icasecmp(compiler.get_name(resources.stage_outputs[0].id), "outColor");
		}
	}


	// (1) build strides for every binding point
	auto &strides = vi.strides;
	strides.clear();
	vi.size = 0;
	for (auto &attr : vi.attribs)
	{
		if (attr.binding >= strides.size())
		{
			auto i = strides.size();
			strides.resize(attr.binding + 1);
			for (; i < strides.size(); ++i)
				strides[i] = 0;
		}
		attr.offset = strides[attr.binding];
		strides[attr.binding] += attr.size;
		vi.size += attr.size;
	}

	// (2) copy calculated strides to vertex description
	for (auto &attr : vi.attribs)
	{
		attr.pStream->m_stride = strides[attr.binding];
	}

	// push constants
	m_layout.pushConstants.clear();
	vi.push_constatns_size = 0;
	for (auto &pc : vi.params_in_push_constants)
	{
		m_layout.pushConstants.push_back({
			pc.stage,
			0,
			pc.entry.size });
		vi.push_constatns_size += pc.entry.size;
	}
	vi.descriptions = BAMS::RENDERINENGINE::GetOptimize()->OptimizeVertexDescription(vi.descriptions);
	
}




