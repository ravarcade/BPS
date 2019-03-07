#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"

using namespace spirv_cross;
using namespace BAMS::RENDERINENGINE;
const char *SharedUniformBufferObject = "UniformBufferObject";

uint32_t FindVertexAttribType(const std::string &name)
{
	static std::vector<std::vector<std::string>> VertexAttribAliases = {
		{ "Vertex", "Position", "inPosition", "Vert" },
		{ "Normal", "inNormal", "Norm" },
		{ "Tangents", "inTangents", "Tang" },
		{ "Bitangents", "inBitangents" "Bitang" },

		{ "Texture",  "Texture1", "inTexture", "inTexture1", "Tex", "Tex1" },
		{ "Texture2", "inTexture2", "Tex2" },
		{ "Texture3", "inTexture3", "Tex3" },
		{ "Texture4", "inTexture4", "Tex4" },

		{ "Color", "Color1", "inColor", "inColor1", "Col", "Col1" },
		{ "Color2", "inColor2", "Col2" },
		{ "Color3", "inColor2", "Col3" },
		{ "Color4", "inColor2", "Col4" },

		{ "BoneWeight", "BWeight", "BoneWeight1", "BWeight1", },
		{ "BoneWeight2", "BWeight2", },
		{ "BoneId", "BId", "BondId1", "BId1" },
		{ "BondId2", "BId2" },
	};

	for (uint32_t i = VertexAttribDesc::VERTEX; i <= VertexAttribDesc::BONEID2; ++i) {
		for (auto &alias : VertexAttribAliases[i]) {
			if (Utils::icasecmp(name, alias)) {
				return i;
			}
		}
	}

	return VertexAttribDesc::UNKNOWN;
}

void FindVertexAttribFormatAndSize(VertexAttribDesc &vd)
{
	static VkFormat f32_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R32_SFLOAT, VK_FORMAT_R32G32_SFLOAT, VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32B32A32_SFLOAT };
	static VkFormat f16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SFLOAT, VK_FORMAT_R16G16_SFLOAT, VK_FORMAT_R16G16B16_SFLOAT, VK_FORMAT_R16G16B16A16_SFLOAT };
	static VkFormat i16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SINT, VK_FORMAT_R16G16_SINT, VK_FORMAT_R16G16B16_SINT, VK_FORMAT_R16G16B16A16_SINT };
	static VkFormat u16_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UINT, VK_FORMAT_R16G16_UINT, VK_FORMAT_R16G16B16_UINT, VK_FORMAT_R16G16B16A16_UINT };
	static VkFormat i8_formats[5]  = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SINT, VK_FORMAT_R8G8_SINT, VK_FORMAT_R8G8B8_SINT, VK_FORMAT_R8G8B8A8_SINT };
	static VkFormat u8_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UINT, VK_FORMAT_R8G8_UINT, VK_FORMAT_R8G8B8_UINT, VK_FORMAT_R8G8B8A8_UINT };
	static VkFormat p10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_UNDEFINED };
	static VkFormat up10_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_UNDEFINED, VK_FORMAT_A2R10G10B10_SNORM_PACK32, VK_FORMAT_A2R10G10B10_UNORM_PACK32, VK_FORMAT_UNDEFINED };
	static VkFormat u8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_UNORM, VK_FORMAT_R8G8_UNORM, VK_FORMAT_R8G8B8_UNORM, VK_FORMAT_R8G8B8A8_UNORM };
	static VkFormat i8n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R8_SNORM, VK_FORMAT_R8G8_SNORM, VK_FORMAT_R8G8B8_SNORM, VK_FORMAT_R8G8B8A8_SNORM };
	static VkFormat i16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_SNORM, VK_FORMAT_R16G16_SNORM, VK_FORMAT_R16G16B16_SNORM, VK_FORMAT_R16G16B16A16_SNORM };
	static VkFormat u16n_formats[5] = { VK_FORMAT_UNDEFINED,  VK_FORMAT_R16_UNORM, VK_FORMAT_R16G16_UNORM, VK_FORMAT_R16G16B16_UNORM, VK_FORMAT_R16G16B16A16_UNORM };
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
	case VertexAttribDesc::VERTEX: // vecsize = 1 and vecsize = 4 are pointless.... but maybe not?
		break;

	case VertexAttribDesc::NORMAL:
	case VertexAttribDesc::TANGENT:
	case VertexAttribDesc::BITANGENT:
		op = o.normals;
		break;

	case VertexAttribDesc::COLOR:
	case VertexAttribDesc::COLOR2:
	case VertexAttribDesc::COLOR3:
	case VertexAttribDesc::COLOR4:
		op = o.colors;
		break;

	case VertexAttribDesc::TEXCOORD:
	case VertexAttribDesc::TEXCOORD2:
	case VertexAttribDesc::TEXCOORD3:
	case VertexAttribDesc::TEXCOORD4:
		op = o.coords;
		break;

	case VertexAttribDesc::BONEWEIGHT:
	case VertexAttribDesc::BONEWEIGHT2:
		op = o.bone_weights;
		break;

	case VertexAttribDesc::BONEID:
	case VertexAttribDesc::BONEID2:
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
	auto shader = rm.GetShaderByName(shaderName);
	if (!shader.IsLoaded())
		rm.LoadSync(shader);
	uint32_t cnt = shader.GetSubprogramsCount();

	m_programs.resize(cnt);

	for (uint32_t i = 0; i < cnt; ++i)
	{
		auto &prg = m_programs[i];
		prg.entryPointName = "main";
		prg.resource = shader.GetSubprogram(i);
		if (!prg.resource.IsLoaded())
			rm.LoadSync(prg.resource);
	}

	_ParsePrograms();
	return vi;
}

// ============================================================================ ShadersReflections private methods ===

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
			vi.attribs.clear();
			for (auto attrib : resources.stage_inputs) 
			{
				VertexAttribDesc vd = {
					compiler.get_decoration(attrib.id, spv::DecorationBinding),
					compiler.get_decoration(attrib.id, spv::DecorationLocation),
					compiler.get_type(attrib.type_id).vecsize,
					FindVertexAttribType(compiler.get_name(attrib.id))
				};

				FindVertexAttribFormatAndSize(vd);

				vi.attribs.push_back(vd);
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

	vi.descriptions = {}; // clear
	CAttribStreamConverter conv;
	for (auto &attr : vi.attribs)
	{
		Stream *s; 
		switch (attr.type) {
		case VertexAttribDesc::VERTEX:      s = &vi.descriptions.m_vertices; break;

		case VertexAttribDesc::NORMAL:      s = &vi.descriptions.m_normals; break;
		case VertexAttribDesc::TANGENT:     s = &vi.descriptions.m_tangents; break;
		case VertexAttribDesc::BITANGENT:   s = &vi.descriptions.m_bitangents; break;

		case VertexAttribDesc::COLOR:       s = &vi.descriptions.m_colors[0]; break;
		case VertexAttribDesc::COLOR2:      s = &vi.descriptions.m_colors[1]; break;
		case VertexAttribDesc::COLOR3:      s = &vi.descriptions.m_colors[2]; break;
		case VertexAttribDesc::COLOR4:      s = &vi.descriptions.m_colors[3]; break;

		case VertexAttribDesc::TEXCOORD:    s = &vi.descriptions.m_textureCoords[0]; break;
		case VertexAttribDesc::TEXCOORD2:   s = &vi.descriptions.m_textureCoords[1]; break;
		case VertexAttribDesc::TEXCOORD3:   s = &vi.descriptions.m_textureCoords[2]; break;
		case VertexAttribDesc::TEXCOORD4:   s = &vi.descriptions.m_textureCoords[3]; break;

		case VertexAttribDesc::BONEWEIGHT:  s = &vi.descriptions.m_boneWeights[0]; break;
		case VertexAttribDesc::BONEWEIGHT2: s = &vi.descriptions.m_boneWeights[1]; break;
		case VertexAttribDesc::BONEID:      s = &vi.descriptions.m_boneIDs[0]; break;
		case VertexAttribDesc::BONEID2:     s = &vi.descriptions.m_boneIDs[1]; break;
		default:
			continue;
		}

		VertexDescriptionInfoPack vdip;
		vdip.data = nullptr;
		vdip.offset  = static_cast<uint8_t>(attr.offset);
		vdip.binding = static_cast<uint8_t>(attr.binding);

		conv.ConvertAttribStreamDescription(*s, VkStream(attr.format, strides[attr.binding], vdip.data));
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

	
}




