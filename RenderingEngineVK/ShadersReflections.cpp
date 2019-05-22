#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"

using namespace spirv_cross;

//const char *SharedUniformBufferObject = "UniformBufferObject";

enum ShaderReflectionType {
	UNKNOWN = 0,
	Int32 = 1,
	UInt32 = 2,
	Int16 = 3,
	UInt16 = 4,
	Int8 = 5,
	UInt8 = 6,
	Float32 = 7,
	Vec2 = 8,
	Vec3 = 9,
	Vec4 = 10,
	Mat3 = 11,
	Mat4 = 12
};

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

	uint32_t toPropType[] = {
		uint32_t(Property::PT_UNKNOWN),
		Property::PT_I32,
		Property::PT_U32,
		Property::PT_I16,
		Property::PT_U16,
		Property::PT_I8,
		Property::PT_U8,
		Property::PT_F32,

		Property::PT_F32, // Vec2,3,4
		Property::PT_F32,
		Property::PT_F32,

		Property::PT_F32, // Mat3, 4
		Property::PT_F32
	};

	uint32_t toPropCount[] = {
		0,
		1, // i32
		1, // u32
		1, // i16
		1, // u16
		1, // i8
		1, // u8
		1, // f32

		2, // vec2
		3, // vec3
		4, // vec4

		9, // mat3
		16, // mat4
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
			mem.rootTypeName = comp.get_name(members[i]);
			mem.offset = comp.type_struct_member_offset(type, i);
			mem.size = static_cast<uint32_t>(comp.get_declared_struct_member_size(type, i));
			mem.array = !member_type.array.empty() ? member_type.array[0] : 1;
			mem.array_stride = !member_type.array.empty() ? comp.type_struct_member_array_stride(type, i) : 0;
			mem.vecsize = member_type.vecsize;
			mem.colsize = member_type.columns;
			mem.matrix_stride = comp.has_decoration(i, spv::DecorationMatrixStride) ? comp.type_struct_member_matrix_stride(type, i) : 0;
			mem.type = typeConv[member_type.basetype];
			SetSimplifiedType(mem);
			mem.propertyType = toPropType[mem.type];
			mem.propertyCount = toPropCount[mem.type];

			if (member_type.basetype == SPIRType::BaseType::Struct && member_type.member_types.size())
			{
				GetDetails(comp, member_type, mem);
			}
		
			// finetune for array of structs:
			if (mem.members.size() > 0 && 
				mem.array > 0 && 
				mem.array_stride > 0) // it is array of struct! so mark it as array
			{
				mem.propertyType  = Property::PT_ARRAY;
				mem.propertyCount = mem.array;
			}

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
	det.entry.rootTypeName = comp.get_name(res.base_type_id);
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
void CShadersReflections::LoadProgram(const char *shaderName)
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

VertexAttribDetails GetVertexAttribDetails(CompilerGLSL &compiler, Resource &attrib, VertexDescription &vd)
{
	VertexAttribDetails vad = {
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
	m_params_in_ubos.clear();
	m_params_in_push_constants.clear();

	m_total_ubos_size = 0;
	m_shared_ubos_size = 0;
	m_ubo_sizes.clear();
	m_sampled_images.clear();

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

		// uniform buffers
		for (auto buffer : resources.uniform_buffers)
		{
			auto set = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
			auto binding = compiler.get_decoration(buffer.id, spv::DecorationBinding);

			ValDetails vd;
			GetDetails(compiler, buffer, vd);
			vd.stage = prg.stage;
			vd.isSharedUBO = vd.entry.rootTypeName == SHAREDUBOTYPENAME;
			vd.isHostVisibleUBO = ForceHostVisibleUBOS || vd.isSharedUBO;
			m_params_in_ubos.push_back(vd);

			if (vd.isSharedUBO)
			{
				m_shared_ubos_size += vd.entry.size;
			}
			else {
				m_total_ubos_size += vd.entry.size;

				if (m_ubo_sizes.size() <= vd.binding)
				{
					size_t i = m_ubo_sizes.size();
					m_ubo_sizes.resize(vd.binding + 1);
					for (; i < vd.binding; ++i)
						m_ubo_sizes[i] = 0;
				}
				m_ubo_sizes[vd.binding] = vd.entry.size;
			}

			auto name = compiler.get_name(buffer.id);
			const SPIRType &type = compiler.get_type(buffer.type_id);
			uint32_t descriptorCount = type.array.size() == 1 ? type.array[0] : 1;

			m_layout.descriptorSets[set].uniform_buffer_mask |= 1u << binding;
			m_layout.descriptorSets[set].descriptorCount[binding] = descriptorCount;
			m_layout.descriptorSets[set].stages[binding] |= prg.stage;
		}

		// push constants
		for (auto pushConst : resources.push_constant_buffers)
		{
			uint32_t size = static_cast<uint32_t>(compiler.get_declared_struct_size(compiler.get_type(pushConst.base_type_id)));
			ValDetails vd;
			GetDetails(compiler, pushConst, vd);
			vd.stage = prg.stage;
			m_params_in_push_constants.push_back(vd);
		}

		// sampled_images
		for (auto resource : resources.sampled_images)
		{
			auto set = compiler.get_decoration(resource.id, spv::DecorationDescriptorSet);
			auto binding = compiler.get_decoration(resource.id, spv::DecorationBinding);
			auto name = compiler.get_name(resource.id);
			const SPIRType &type = compiler.get_type(resource.type_id);
			uint32_t descriptorCount = type.array.size() == 1 ? type.array[0] : 1;
			assert(type.basetype == SPIRType::SampledImage);
			if (type.basetype == SPIRType::SampledImage)
			{
				m_sampled_images.push_back({set, binding, name, static_cast<uint32_t>(type.image.dim), static_cast<uint32_t>(prg.stage)});
				m_layout.descriptorSets[set].sampled_image_mask |= 1u << binding;
				m_layout.descriptorSets[set].descriptorCount[binding] = descriptorCount;
				m_layout.descriptorSets[set].stages[binding] |= prg.stage;
			}
		}

		// for vertex stage get attributes
		if (prg.stage == VK_SHADER_STAGE_VERTEX_BIT)
		{
			m_vertexAttribs.attribs.clear();
			m_vertexDescription = {}; // clear
			for (auto attrib : resources.stage_inputs) 
			{
				VertexAttribDetails vad = GetVertexAttribDetails(compiler, attrib, m_vertexDescription);
				m_vertexAttribs.attribs.push_back(vad);
			}
		}

		// for fragment stage get target (outputNames)
		if (prg.stage == VK_SHADER_STAGE_FRAGMENT_BIT) 
		{
			m_outputNames.clear();
			for (uint32_t i = 0; i < resources.stage_outputs.size(); ++i)
			{
				m_outputNames.push_back(compiler.get_name(resources.stage_outputs[i].id));
			}
			m_enableAlpha = resources.stage_outputs.size() == 1 && Utils::icasecmp(compiler.get_name(resources.stage_outputs[0].id), "outColor");
		}
	}


	// (1) build strides for every binding point
	auto &strides = m_vertexAttribs.strides;
	strides.clear();
	m_vertexAttribs.size = 0;
	for (auto &attr : m_vertexAttribs.attribs)
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
		m_vertexAttribs.size += attr.size;
	}

	// (2) copy calculated strides to vertex description
	for (auto &attr : m_vertexAttribs.attribs)
	{
		attr.pStream->m_stride = strides[attr.binding];
	}

	// push constants
	m_layout.pushConstants.clear();
	for (auto &pc : m_params_in_push_constants)
	{
		m_layout.pushConstants.push_back({
			pc.stage,
			0,
			pc.entry.size });
	}
	m_vertexDescription = GetOptimize()->OptimizeVertexDescription(m_vertexDescription);
	m_properties = _BuildProperties();
}

struct ShaderProgramParamDesc {
	const ValDetails *root;
	const ValMemberDetails *mem;
	uint32_t parentIdx;
	uint32_t dataBufferId;
	uint32_t propertyParentIdx;
};

void _BuindShaderProgramParamsDesc(std::vector<ShaderProgramParamDesc> &shaderProgramParamNames, const ValMemberDetails &entry, uint32_t parentIdx, const ValDetails &root, uint32_t dataBufferId)
{
	assert(entry.members.size() > 0);

	for (auto &mem : entry.members)
	{
		if (mem.members.size())
		{
			// add parent?
			if (mem.propertyType == Property::PT_ARRAY) // it is array! add parrent
			{
				shaderProgramParamNames.push_back({
					&root,
					&mem,
					parentIdx,
					dataBufferId,
					static_cast<uint32_t>(-1)
					});
				parentIdx = static_cast<uint32_t>(shaderProgramParamNames.size() - 1);
			}
			_BuindShaderProgramParamsDesc(shaderProgramParamNames, mem, parentIdx, root, dataBufferId);
		}
		else if (mem.propertyType != Property::PT_UNKNOWN)
		{
			shaderProgramParamNames.push_back({
				&root,
				&mem,
				parentIdx,
				dataBufferId,
				static_cast<uint32_t>(-1)
				});
		}
	}
}

MProperties CShadersReflections::_BuildProperties()
{
	std::vector<ShaderProgramParamDesc> shaderProgramParamNames;

	uint32_t dataBufferId = 0;
	for (auto &vd : m_params_in_push_constants) {
		_BuindShaderProgramParamsDesc(shaderProgramParamNames, vd.entry, -1, vd, dataBufferId);
		++dataBufferId;
	}
	for (auto &vd : m_params_in_ubos) {
		_BuindShaderProgramParamsDesc(shaderProgramParamNames, vd.entry, -1, vd, dataBufferId);
		++dataBufferId;
	}

	auto m_isPushConstantsUsed = m_params_in_push_constants.size() && shaderProgramParamNames.size(); // !!!


	// ------------------------------------
	// we want to skip "SharedUBO"
	MProperties	properties;

	// add all parents for lev 0
	uint32_t paramIdx = -1;
	for (auto &p : shaderProgramParamNames)
	{
		++paramIdx;
		if (p.root->isSharedUBO)
			continue;

		p.propertyParentIdx = paramIdx;
		uint32_t parentId = -1;
		if (p.parentIdx < shaderProgramParamNames.size())
		{
			parentId = shaderProgramParamNames[p.parentIdx].propertyParentIdx;
		}

		// add entry
		auto pr = properties.add();
		pr->name = p.mem->name.c_str();
		pr->parent = parentId;
		pr->type = p.mem->propertyType;
		pr->count = p.mem->propertyCount;
		pr->array_size = p.mem->array;
		pr->array_stride = p.mem->array_stride;
		pr->buffer_idx = p.dataBufferId;
		pr->buffer_object_stride = p.root->entry.size;
		pr->buffer_offset = p.mem->offset;
	}

	// add textures
	U32 sampledImagesBufferIdx = static_cast<U32>(m_params_in_push_constants.size() + m_params_in_ubos.size());
	U32 sampledImagesBufferObjectStrid = static_cast<U32>(m_sampled_images.size() * sizeof(CTexture2d*));
	U32 sampledImagesBufferOffset = 0;
	for (auto &si : m_sampled_images)
	{
		auto pr = properties.add();
		pr->name = si.name.c_str();
		pr->parent = -1;
		pr->type = Property::PT_TEXTURE;
		pr->count = 1;
		pr->array_size = 0;
		pr->array_stride = 0;
		pr->val = 0;
		pr->buffer_idx = sampledImagesBufferIdx;
		pr->buffer_object_stride = sampledImagesBufferObjectStrid;
		pr->buffer_offset = sampledImagesBufferOffset;
		sampledImagesBufferOffset += sizeof(CTexture2d*);
	}

	return std::move(properties);
}


