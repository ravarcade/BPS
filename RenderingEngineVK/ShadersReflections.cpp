#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"

using namespace spirv_cross;
using namespace BAMS::RENDERINENGINE;

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

void GetDetails(CompilerGLSL &comp, Resource &res, ValDetails &det)
{

	ShaderReflectionType typeConv[] = {
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

	uint32_t typeSize[] = {
		0, 0, 0, 1, 4,
		4, 8, 8, 0, 2,
		4, 8, 0, 0, 0,
		0
	};

	det = {};
	auto &ent = det.entry;
	SPIRType type = comp.get_type(res.type_id);
	auto bitset = comp.get_decoration_bitset(res.type_id);
	
	det.set     = comp.has_decoration(res.id, spv::DecorationDescriptorSet) ? det.set = comp.get_decoration(res.id, spv::DecorationDescriptorSet) : 0;
	det.binding = comp.has_decoration(res.id, spv::DecorationBinding) ? comp.get_decoration(res.id, spv::DecorationBinding) : 0;
	ent.name = comp.get_name(res.id);
	ent.array = type.array.size() == 1 ? type.array[0] : 1;
	ent.vecsize = type.vecsize;
	ent.colsize = type.columns;

	if (type.basetype == SPIRType::BaseType::Struct && type.member_types.size())
	{
		ent.size = static_cast<uint32_t>(comp.get_declared_struct_size(comp.get_type(res.base_type_id)));
		auto &members = type.member_types;
		for (uint32_t i = 0; i < members.size(); ++i) {
			SPIRType member_type = comp.get_type(members[i]);
			ValMemberDetails mem;
			mem.name   = comp.get_member_name(type.self, i);
			mem.offset = comp.type_struct_member_offset(type, i);
			mem.size   = static_cast<uint32_t>(comp.get_declared_struct_member_size(type, i));
			mem.array = !member_type.array.empty() ? member_type.array[0] : 1;
			mem.array_stride = !member_type.array.empty() ? comp.type_struct_member_array_stride(type, i) : 0;
			mem.vecsize = member_type.vecsize;
			mem.colsize = member_type.columns;
			mem.matrix_stride = member_type.columns ? comp.type_struct_member_matrix_stride(type, i) : 0;
			mem.type = typeConv[member_type.basetype];
			det.members.push_back(mem);
		}
	}
	else if (typeConv[type.basetype] != ShaderReflectionType::UNKNOWN) 
	{
		ent.type = typeConv[type.basetype];
		ent.size = typeSize[type.basetype] * ent.vecsize * ent.colsize * ent.array;
	}
}

ShadersReflections::ShadersReflections()
{}

ShadersReflections::ShadersReflections(std::vector<std::string> &&programs) 
{
	LoadPrograms(std::move(programs));
}

void ShadersReflections::LoadPrograms(std::vector<std::string>&& programs)
{
	m_programs.resize(programs.size());
	BAMS::CResourceManager rm;
	bool needToLoadResources = false;

	for (uint32_t i = 0; i < programs.size(); ++i)
	{
		auto &prg = m_programs[i];
		prg.name = programs[i];
		prg.entryPointName = "main";
		prg.resource = rm.GetRawDataByName(prg.name.c_str());
		if (!prg.resource.IsLoaded())
			needToLoadResources = true;
	}

	if (needToLoadResources)
		rm.LoadSync();

	ParsePrograms();
}

void ShadersReflections::ParsePrograms()
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
			m_ubos.push_back(vd);

			auto name = compiler.get_name(buffer.id);
			SPIRType type = compiler.get_type(buffer.type_id);
			uint32_t descriptorCount = type.array.size() == 1 ? type.array[0] : 1;

			m_layout.descriptorSets[set].uniform_buffer_mask |= 1u << binding;
			m_layout.descriptorSets[set].descriptorCount[binding] = descriptorCount;
			m_layout.descriptorSets[set].stages[binding] |= prg.stage;
		}

		if (prg.stage == VK_SHADER_STAGE_VERTEX_BIT) 
		{
			m_VertexAttribs.clear();
			for (auto attrib : resources.stage_inputs) 
			{
				VertexAttribDesc vd = {
					compiler.get_decoration(attrib.id, spv::DecorationBinding),
					compiler.get_decoration(attrib.id, spv::DecorationLocation),
					compiler.get_type(attrib.type_id).vecsize,
					FindVertexAttribType(compiler.get_name(attrib.id))
				};

				FindVertexAttribFormatAndSize(vd);

				m_VertexAttribs.push_back(vd);
			}
		}

		if (prg.stage == VK_SHADER_STAGE_FRAGMENT_BIT) 
		{
			m_enableAlpha = resources.stage_outputs.size() == 1 && Utils::icasecmp(compiler.get_name(resources.stage_outputs[0].id), "outColor");
		}
	}
}

void ShadersReflections::CreatePipelineLayout(VkDevice device, const VkAllocationCallbacks* allocator)
{
	m_descriptorSetLayout.clear();
	
	uint32_t lastSet = 0, i = 0;
	for (auto &set : m_layout.descriptorSets)
	{
		if (set.uniform_buffer_mask)
			lastSet = i;
		++i;
	}
	
	for (uint32_t setNo = 0; setNo <= lastSet; ++setNo)
	{
		auto &set = m_layout.descriptorSets[setNo];
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		if (set.uniform_buffer_mask)
		{
			for (unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
			{
				uint32_t bitMask = 1u << i;
				uint32_t descriptorCount = set.descriptorCount[i];
				uint32_t stages = set.stages[i];

				if (set.uniform_buffer_mask & bitMask) {
					bindings.push_back({ i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount, stages, nullptr });
				}
			}
		}

		if (bindings.size())
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			VkDescriptorSetLayout descriptorSetLayout;
			if (vkCreateDescriptorSetLayout(device, &layoutInfo, allocator, &descriptorSetLayout) != VK_SUCCESS)
				throw std::runtime_error("failed to create descriptor set layout!");

			m_descriptorSetLayout.push_back(descriptorSetLayout);
		}
	}

	// add empty set
	if (false) {
		VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
		layoutInfo.bindingCount = 0;
		layoutInfo.pBindings = nullptr;
		VkDescriptorSetLayout descriptorSetLayout;
		if (vkCreateDescriptorSetLayout(device, &layoutInfo, allocator, &descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");
		m_descriptorSetLayout.push_back(descriptorSetLayout);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO  };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

	if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, allocator, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}

}

void ShadersReflections::ReleaseVk(VkDevice device, const VkAllocationCallbacks * allocator)
{
	for (auto &dsl : m_descriptorSetLayout) {
		vkDestroyDescriptorSetLayout(device, dsl, allocator);
	}

	m_descriptorSetLayout.clear();
	vkDestroyPipelineLayout(device, m_pipelineLayout, allocator);
	m_pipelineLayout = nullptr;
	if (m_graphicsPipeline)
		vkDestroyPipeline(device, m_graphicsPipeline, allocator);
	m_graphicsPipeline = nullptr;
}

uint32_t ShadersReflections::AddDescriptorPoolsSize(std::vector<uint32_t>& poolsSize)
{
	if (poolsSize.size() < VK_DESCRIPTOR_TYPE_RANGE_SIZE) {
		uint32_t i = static_cast<uint32_t>(poolsSize.size());
		poolsSize.resize(VK_DESCRIPTOR_TYPE_RANGE_SIZE);
		for (; i < poolsSize.size(); ++i) {
			poolsSize[i] = 0;
		}
	}

	for (auto &set : m_layout.descriptorSets) {
		if (set.uniform_buffer_mask) {
			poolsSize[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += Utils::count_bits(set.uniform_buffer_mask);
		}
	}

	uint32_t nonZero = 0;
	for (auto x : poolsSize) {
		if (x > 0)
			++nonZero;
	}

	return nonZero;
}

VkPipelineVertexInputStateCreateInfo *ShadersReflections::GetVertexInputInfo()
{
	m_bindingDescription.clear();
	m_attributeDescriptions.clear();
	uint32_t maxBinding = 0;
	for (uint32_t binding = 0; binding <= maxBinding; ++binding)
	{
		uint32_t stride = 0;
		for (auto &attr : m_VertexAttribs) 
		{
			if (maxBinding < attr.binding)
				maxBinding = attr.binding;

			if (attr.binding == binding) {
				attr.offset = stride;
				stride += attr.size;
			}
		}

		m_bindingDescription.push_back({
			binding,
			stride,
			VK_VERTEX_INPUT_RATE_VERTEX
		});
	}
	for (auto &attr : m_VertexAttribs)
	{
		m_attributeDescriptions.push_back({
			attr.location,
			attr.binding,
			attr.format,
			attr.offset
		});
	}

	m_vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	m_vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindingDescription.size());
	m_vertexInputInfo.pVertexBindingDescriptions = m_bindingDescription.data();

	m_vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
	m_vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();

	return &m_vertexInputInfo;
}

VkPipeline ShadersReflections::CreateGraphicsPipeline(VkDevice device, const VkAllocationCallbacks * allocator)
{
	logicalDevice = device;
	memAllocator = allocator;

	m_graphicsPipeline = nullptr;

	auto shaderStages    = _Compile();
	auto vertexInputInfo = _GetVertexInputInfo();
	auto inputAssembly   = _GetInputAssembly();
	auto viewportState   = _GetViewportState();
	auto rasterizer      = _GetRasterizationState();
	auto multisampling   = _GetMultisampleState();
	auto depthStencil    = _GetDepthStencilState();
	auto colorBlending   = _GetColorBlendState();
	auto dynamicState    = _GetDynamicState();
	auto pipelineLayout  = _GetPipelineLayout();
	auto renderPass      = _GetRenderPass();

	VkGraphicsPipelineCreateInfo pipelineInfo = { VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO };
	pipelineInfo.stageCount = (uint32_t)shaderStages.size();
	pipelineInfo.pStages = shaderStages.data();
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = &dynamicState;
	pipelineInfo.layout = pipelineLayout;
	pipelineInfo.renderPass = renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional

	if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, allocator, &m_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	// some cleanups
	for (auto shaderStage : shaderStages)
		vkDestroyShaderModule(logicalDevice, shaderStage.module, memAllocator);

	return m_graphicsPipeline;
}


// ============================================================================ ShadersReflections private methods ===

std::vector<VkPipelineShaderStageCreateInfo> ShadersReflections::_Compile()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	shaderStages.resize(m_programs.size());
	uint32_t idx = 0;

	for (auto &prg : m_programs) {
		auto code = prg.resource;

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.GetSize();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.GetData());
		if (vkCreateShaderModule(logicalDevice, &createInfo, memAllocator, &prg.shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		auto &stageCreateInfo = shaderStages[idx];
		stageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageCreateInfo.stage = prg.stage;
		stageCreateInfo.module = prg.shaderModule;
		stageCreateInfo.pName = prg.entryPointName.c_str();

		++idx;
	}

	return shaderStages;
}

VkPipelineVertexInputStateCreateInfo ShadersReflections::_GetVertexInputInfo()
{
	m_bindingDescription.clear();
	m_attributeDescriptions.clear();

	uint32_t maxBinding = 0;
	for (uint32_t binding = 0; binding <= maxBinding; ++binding)
	{
		uint32_t stride = 0;
		for (auto &attr : m_VertexAttribs)
		{
			if (maxBinding < attr.binding)
				maxBinding = attr.binding;

			if (attr.binding == binding) {
				attr.offset = stride;
				stride += attr.size;
			}
		}

		m_bindingDescription.push_back({
			binding,
			stride,
			VK_VERTEX_INPUT_RATE_VERTEX
		});
	}
	for (auto &attr : m_VertexAttribs)
	{
		m_attributeDescriptions.push_back({
			attr.location,
			attr.binding,
			attr.format,
			attr.offset
		});
	}

	VkPipelineVertexInputStateCreateInfo vertexInputInfo = { VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO };
	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(m_bindingDescription.size());
	vertexInputInfo.pVertexBindingDescriptions = m_bindingDescription.data();

	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(m_attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = m_attributeDescriptions.data();

	return vertexInputInfo;
}

VkPipelineInputAssemblyStateCreateInfo ShadersReflections::_GetInputAssembly()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	return  inputAssembly;
}

VkPipelineViewportStateCreateInfo ShadersReflections::_GetViewportState()
{
	static VkExtent2D swapChainExtent = { 1024, 768 };
	static VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// no scisors
	static VkRect2D scissor = { { 0, 0 }, swapChainExtent };

	// viewport and scisors in one structure: viewportState
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	return viewportState;
}

VkPipelineRasterizationStateCreateInfo ShadersReflections::_GetRasterizationState()
{
	VkPipelineRasterizationStateCreateInfo rasterizer = { VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO };
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	return rasterizer;
}

VkPipelineMultisampleStateCreateInfo ShadersReflections::_GetMultisampleState()
{
	VkPipelineMultisampleStateCreateInfo multisampling = { VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO };
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = m_msaaSamples;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	return multisampling;
}

VkPipelineDepthStencilStateCreateInfo ShadersReflections::_GetDepthStencilState()
{
	VkPipelineDepthStencilStateCreateInfo depthStencil = { VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO };
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	return depthStencil;
}

VkPipelineColorBlendStateCreateInfo ShadersReflections::_GetColorBlendState()
{
	static std::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;
	colorBlendAttachments.clear();

	/* TODO: Add support for multiple attachment. Right now all works only with one. */

	VkPipelineColorBlendAttachmentState colorBlendAttachment;
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachments.push_back(colorBlendAttachment);

	/* alpha blending
	colorBlendAttachment.blendEnable = VK_TRUE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
	*/

	VkPipelineColorBlendStateCreateInfo colorBlending = { VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO };
	colorBlending.flags = 0;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = static_cast<uint32_t>(colorBlendAttachments.size());
	colorBlending.pAttachments = colorBlendAttachments.data();
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	return colorBlending;
}

VkPipelineDynamicStateCreateInfo ShadersReflections::_GetDynamicState()
{
	// right now we don't set viewport and scissors in pipline creation

	static std::vector<VkDynamicState> dynamicStates = {
		VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
	dynamicState.pDynamicStates = dynamicStates.data();

	return dynamicState;
}

VkPipelineLayout ShadersReflections::_GetPipelineLayout()
{
	if (m_pipelineLayout == nullptr)
	{
		CreatePipelineLayout(logicalDevice, memAllocator);
	}

	return m_pipelineLayout;
}

VkRenderPass ShadersReflections::_GetRenderPass()
{
	return m_renderPass;
}
