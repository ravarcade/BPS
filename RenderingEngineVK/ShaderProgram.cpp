#include "stdafx.h"

using namespace BAMS;
using namespace BAMS::RENDERINENGINE;
const uint32_t ALWAYS_RESERVE_VERTICES = 16 * 1024; // <- 256kB for 16bytes / per vertice
const uint32_t ALWAYS_RESERVE_INDICES = ALWAYS_RESERVE_VERTICES;

void CShaderProgram::LoadProgram(const char *program)
{
	// to do: add support for loading program description from manifest
	m_vi = m_reflection.LoadProgram(program);
	_BuindShaderProgramParamsDesc();
	_BuindShaderDataBuffers();
}

void CShaderProgram::Release()
{
	for (auto &ub : m_uniformBuffers)
		vk->vkDestroy(ub);
	m_uniformBuffers.clear();

	for (auto &ub : m_uniformBuffersMemory)
		vk->vkFree(ub);
	m_uniformBuffersMemory.clear();

	for (auto &set : m_bufferSets) {
		vk->vkDestroy(set.vertexBuffer);
		vk->vkFree(set.vertexBufferMemory);
		vk->vkDestroy(set.indexBuffer);
		vk->vkFree(set.indexBufferMemory);
	}
	m_bufferSets.clear();

	for (auto &dsl : m_descriptorSetLayout) {
		vk->vkDestroy(dsl);
	}
	m_descriptorSetLayout.clear();

	vk->vkDestroy(m_pipelineLayout);
	vk->vkDestroy(m_graphicsPipeline);
}

const std::vector<std::string> &CShaderProgram::GetOutputNames()
{ 
	return m_reflection.GetOutputNames(); 
}

void CShaderProgram::SetRenderPassAndMsaaSamples(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples)
{ 
	m_renderPass = renderPass; 
	m_msaaSamples = msaaSamples; 
}

VkPipeline CShaderProgram::CreateGraphicsPipeline()
{
	m_graphicsPipeline = nullptr;

	auto shaderStages = _Compile();
	auto vertexInputInfo = _GetVertexInputInfo();
	auto inputAssembly = _GetInputAssembly();
	auto viewportState = _GetViewportState();
	auto rasterizer = _GetRasterizationState();
	auto multisampling = _GetMultisampleState();
	auto depthStencil = _GetDepthStencilState();
	auto colorBlending = _GetColorBlendState();
	auto dynamicState = _GetDynamicState();
	auto pipelineLayout = _GetPipelineLayout();
	auto renderPass = _GetRenderPass();

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

	if (vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, vk->allocator, &m_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	// some cleanups
	for (auto shaderStage : shaderStages)
		vk->vkDestroy(shaderStage.module);

	// other stuffs use by shader program
	_CreateUniformBuffers();
	return m_graphicsPipeline;
}

// ============================================================================ CShaderProgram private methods ===

std::vector<VkPipelineShaderStageCreateInfo> CShaderProgram::_Compile()
{
	std::vector<VkPipelineShaderStageCreateInfo> shaderStages;

	auto &programs = m_reflection.GetPrograms();
	shaderStages.resize(programs.size());
	uint32_t idx = 0;

	for (auto &prg : programs) {
		auto code = prg.resource;
		auto &stageCreateInfo = shaderStages[idx];
		stageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageCreateInfo.stage = prg.stage;
		stageCreateInfo.pName = prg.entryPointName.c_str();

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.GetSize();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.GetData());
		if (vkCreateShaderModule(vk->device, &createInfo, vk->allocator, &stageCreateInfo.module) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}


		++idx;
	}

	return shaderStages;
}

VkPipelineVertexInputStateCreateInfo CShaderProgram::_GetVertexInputInfo()
{
	m_bindingDescription.clear();
	m_attributeDescriptions.clear();

	for (uint32_t binding = 0; binding < m_vi.strides.size(); ++binding)
	{
		auto stride = m_vi.strides[binding];
		if (stride > 0)
		{
			m_bindingDescription.push_back({
				binding,
				stride,
				VK_VERTEX_INPUT_RATE_VERTEX
			});
		}
	}

	for (auto &attr : m_vi.attribs)
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

VkPipelineInputAssemblyStateCreateInfo CShaderProgram::_GetInputAssembly()
{
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = { VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO };
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	return  inputAssembly;
}

VkPipelineViewportStateCreateInfo CShaderProgram::_GetViewportState()
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
	static VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	return viewportState;
}

VkPipelineRasterizationStateCreateInfo CShaderProgram::_GetRasterizationState()
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

VkPipelineMultisampleStateCreateInfo CShaderProgram::_GetMultisampleState()
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

VkPipelineDepthStencilStateCreateInfo CShaderProgram::_GetDepthStencilState()
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

VkPipelineColorBlendStateCreateInfo CShaderProgram::_GetColorBlendState()
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

VkPipelineDynamicStateCreateInfo CShaderProgram::_GetDynamicState()
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

VkPipelineLayout CShaderProgram::_GetPipelineLayout()
{
	if (m_pipelineLayout == nullptr)
	{
		CreatePipelineLayout();
	}

	return m_pipelineLayout;
}

VkRenderPass CShaderProgram::_GetRenderPass()
{
	return m_renderPass;
}

uint32_t CShaderProgram::_GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize)
{
	if (poolsSize.size() < VK_DESCRIPTOR_TYPE_RANGE_SIZE) {
		uint32_t i = static_cast<uint32_t>(poolsSize.size());
		poolsSize.resize(VK_DESCRIPTOR_TYPE_RANGE_SIZE);
		for (; i < poolsSize.size(); ++i) {
			poolsSize[i] = 0;
		}
	}

	auto layout = m_reflection.GetLayout();

	for (auto &set : layout.descriptorSets) {
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

void CShaderProgram::CreatePipelineLayout()
{
	m_descriptorSetLayout.clear();

	uint32_t lastSet = 0, i = 0;
	auto layout = m_reflection.GetLayout();
	for (auto &set : layout.descriptorSets)
	{
		if (set.uniform_buffer_mask)
			lastSet = i;
		++i;
	}

	for (uint32_t setNo = 0; setNo <= lastSet; ++setNo)
	{
		auto &set = layout.descriptorSets[setNo];
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
			if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, vk->allocator, &descriptorSetLayout) != VK_SUCCESS)
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
		if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, vk->allocator, &descriptorSetLayout) != VK_SUCCESS)
			throw std::runtime_error("failed to create descriptor set layout!");
		m_descriptorSetLayout.push_back(descriptorSetLayout);
	}

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = { VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO };
	pipelineLayoutInfo.setLayoutCount = static_cast<uint32_t>(m_descriptorSetLayout.size());
	pipelineLayoutInfo.pSetLayouts = m_descriptorSetLayout.data();
	pipelineLayoutInfo.pushConstantRangeCount = static_cast<uint32_t>(layout.pushConstants.size());
	pipelineLayoutInfo.pPushConstantRanges = layout.pushConstants.data();

	if (vkCreatePipelineLayout(vk->device, &pipelineLayoutInfo, vk->allocator, &m_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}
}

void CShaderProgram::_CreateNewBufferSet(uint32_t numVertices, uint32_t numIndeces)
{
	BufferSet set;		

	vk->_CreateBuffer(
		numVertices * m_vi.size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		set.vertexBuffer,
		set.vertexBufferMemory);

	vk->_CreateBuffer(
		numIndeces * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		set.indexBuffer, 
		set.indexBufferMemory);

	set.usedVertices = 0;
	set.usedIndices = 0;
	set.freeVertices = numVertices;
	set.freeIndices = numIndeces;

	m_bufferSets.emplace_back(set);
}

uint32_t CShaderProgram::AddModel(const VertexDescription &src)
{
	// we check only last BufferSet
	BufferSet *set = m_bufferSets.size() ? &*m_bufferSets.rbegin() : nullptr;
	if (set == nullptr || set->freeVertices < src.m_numVertices || set->freeIndices < src.m_numIndices)
	{
		_CreateNewBufferSet(
			src.m_numVertices < ALWAYS_RESERVE_VERTICES ? src.m_numVertices + ALWAYS_RESERVE_VERTICES : src.m_numVertices,
			src.m_numIndices < ALWAYS_RESERVE_INDICES ? src.m_numIndices + ALWAYS_RESERVE_INDICES : src.m_numIndices);
		set = &*m_bufferSets.rbegin();
	}
	uint32_t bufferSetIdx = static_cast<uint32_t>(m_bufferSets.size()) - 1;

	uint32_t indeceSize = sizeof(uint32_t);
	uint32_t firstIndex = set->usedIndices;
	uint32_t vboSize = src.m_numVertices * m_vi.size;
	uint32_t iboSize = src.m_numIndices * indeceSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->_CreateBuffer(
		vboSize + iboSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->device, stagingBufferMemory, 0, vboSize + iboSize, 0, &data);
	_ImportModelData(src, data);
	vkUnmapMemory(vk->device, stagingBufferMemory);

	vk->_CopyBuffer(stagingBuffer, set->vertexBuffer, vboSize, 0, set->usedVertices * m_vi.size);
	vk->_CopyBuffer(stagingBuffer, set->indexBuffer, iboSize, vboSize, set->usedIndices * indeceSize);

	vk->vkDestroy(stagingBuffer);
	vk->vkFree(stagingBufferMemory);

	// add "model"
	Model m = {
		bufferSetIdx,
		set->usedVertices,
		set->usedIndices,
		src.m_numVertices,
		src.m_numIndices
	};

	set->usedVertices += src.m_numVertices;
	set->usedIndices += src.m_numIndices;

	m_models.push_back(m);

	return static_cast<uint32_t>(m_models.size()) - 1;
}

uint32_t CShaderProgram::AddObject(uint32_t modeIdx)
{
	DrawObjectData dod;
	dod.modelIdx = modeIdx;
	dod.paramsSetId = static_cast<uint32_t>(m_drawObjectData.size());
	m_drawObjectData.push_back(dod);

	return static_cast<uint32_t>(m_drawObjectData.size()) - 1;
}

void CShaderProgram::_SendVertexStream(BAMS::RENDERINENGINE::Stream dst, BAMS::RENDERINENGINE::Stream & src, uint8_t *outBuf, std::vector<uint32_t> &bindingOffset, uint32_t numVertices)
{
	static CAttribStreamConverter conv;

	if(src.isUsed())
	{
		VertexDescriptionInfoPack vdip;
		vdip.data = dst.m_data;
		dst.m_data = outBuf + vdip.offset + bindingOffset[vdip.binding];
		conv.Convert(dst, src, numVertices);
	}
}

void CShaderProgram::_CreateUniformBuffers()
{
	for (auto &ubo : m_vi.params_in_ubos)
	{
		if (ubo.entry.name == "ubo")
			continue;
		VkDeviceSize bufferSize = ubo.entry.size;
		VkBuffer ubo;
		VkDeviceMemory ubomem;

		vk->_CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			ubo, ubomem);

		m_uniformBuffers.push_back(ubo);
		m_uniformBuffersMemory.push_back(ubomem);
	}
}

void CShaderProgram::_BuindShaderProgramParamsDesc()
{
	m_shaderProgramParamNames.clear();
	uint32_t dataBufferId = 0;
	_BuindShaderProgramParamsDesc(&m_vi.params_in_push_constants, dataBufferId);
	_BuindShaderProgramParamsDesc(&m_vi.params_in_ubos, dataBufferId);
	m_shaderProgramParamsDesc = { static_cast<uint32_t>(m_shaderProgramParamNames.size()), m_shaderProgramParamNames.data() };
}

void CShaderProgram::_BuindShaderProgramParamsDesc(const std::vector<ValDetails>* vals, uint32_t &dataBufferId)
{
	for (auto &vd : *vals) {
		_BuindShaderProgramParamsDesc(vd.entry, vd.entry.name.c_str(), vd.entry.size, dataBufferId);
		++dataBufferId;
	}
}

void CShaderProgram::_BuindShaderProgramParamsDesc(const ValMemberDetails &entry, const char *parentName, uint32_t stride, uint32_t dataBufferId)
{
	if (entry.members.size())
	{
		for (auto &mem : entry.members)
		{
			if (mem.members.size())
			{
				_BuindShaderProgramParamsDesc(mem, parentName, stride, dataBufferId);
			}
			else if (mem.type != ShaderReflectionType::UNKNOWN)
			{
				m_shaderProgramParamNames.push_back({
					mem.type,
					mem.name.c_str(),
					parentName,
					mem.offset,
					mem.size,
					stride,
					dataBufferId
					});
			}
		}
	}
	else {
		m_shaderProgramParamNames.push_back({
			entry.type,
			parentName,
			nullptr,
			entry.offset,
			entry.size,
			stride,
			dataBufferId
			});
	}
}

uint32_t MAXBUFFERCHUNKSIZE = 32 * 1024;

void CShaderProgram::_BuindShaderDataBuffers()
{
	uint32_t maxObjects = MAXBUFFERCHUNKSIZE / (m_vi.push_constatns_size  > m_vi.max_single_ubo_size ? m_vi.push_constatns_size : m_vi.max_single_ubo_size);

	m_paramsBuffer.resize(m_vi.params_in_push_constants.size() + m_vi.params_in_ubos.size());
	for (auto &buf : m_paramsBuffer)
	{
		buf.resize(MAXBUFFERCHUNKSIZE);
	}
}

void CShaderProgram::_ImportModelData(VertexDescription src, void *dst)
{
	uint32_t vboSize = 0; 
	uint32_t iboSize = src.m_numIndices * sizeof(uint32_t);

	std::vector<uint32_t> bindingOffset;
	bindingOffset.push_back(0);

	for (auto ps : m_vi.strides) {
		vboSize += src.m_numVertices * ps;
		bindingOffset.push_back(vboSize);
	}

	assert(vboSize == src.m_numVertices * m_vi.size);

	CAttribStreamConverter conv;

	auto &d = m_vi.descriptions;
	auto outBuf = reinterpret_cast<uint8_t *>(dst);
#define SENDVERTEXSTREAM(x) _SendVertexStream(d.m_ ## x, src.m_ ## x, outBuf, bindingOffset, src.m_numVertices);
	SENDVERTEXSTREAM(vertices);
	SENDVERTEXSTREAM(colors[0]);
#undef SENDVERTEXSTREAM

	Stream out((U32)IDX_UINT32_1D, sizeof(uint32_t), false, reinterpret_cast<uint8_t *>(dst) + vboSize);
	conv.Convert(out, src.m_indices, src.m_numIndices);
}

void CShaderProgram::CreateDescriptorSets()
{
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = vk->descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(m_descriptorSetLayout.size());
	allocInfo.pSetLayouts = m_descriptorSetLayout.data();

	if (vkAllocateDescriptorSets(vk->device, &allocInfo, &m_descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
}

void CShaderProgram::UpdateDescriptorSets()
{

	std::vector<VkDescriptorBufferInfo> buffersInfo;


	buffersInfo.push_back({
		vk->sharedUniformBuffer,
		0,
		sizeof(OutputWindow::SharedUniformBufferObject) });
	/*
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _textureImageView;
	imageInfo.sampler = _textureSampler;
	*/

	/*	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = _descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = _descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;
	*/
	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = m_descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = static_cast<uint32_t>(buffersInfo.size());
	descriptorWrites[0].pBufferInfo = buffersInfo.data();

	vkUpdateDescriptorSets(vk->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

void CShaderProgram::DrawObjects(VkCommandBuffer & cb)
{
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	vkCmdSetViewport(cb, 0, 1, &vk->viewport);
	vkCmdSetScissor(cb, 0, 1, &vk->scissor);

	/*
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	static struct {
		glm::mat4 model;
		glm::vec4 baseColor;
	} pushConstantData;
	pushConstantData.model = glm::rotate(glm::translate(glm::mat4(1.0f), glm::vec3(objectId * 2 - 2.0, 0, 0)), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	pushConstantData.baseColor = glm::vec4(1, 0.5, 1, 1);
	*/
	uint32_t lastBufferSetIdx = -1;
	if (vk->currentDescriptorSet != m_descriptorSet)
	{
		vk->currentDescriptorSet = m_descriptorSet;
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &vk->currentDescriptorSet, 0, nullptr);
	}

	uint32_t objectId = 0;
	for (auto &dod : m_drawObjectData)
	{
		
		auto &m = m_models[dod.modelIdx];
		if (m.bufferSetIdx != lastBufferSetIdx)
		{
			auto &bs = m_bufferSets[m.bufferSetIdx];
			VkBuffer vertexBuffers[] = { bs.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(cb, bs.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		vkCmdPushConstants(cb, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 80, m_paramsBuffer[0].data() + 80 * objectId);
		vkCmdDrawIndexed(cb, m.numIndex, 1, m.firstIndex, m.firstVertex, 0);
		++objectId;
	}
}

uint32_t CShaderProgram::GetParamId(const char * name, const char * parentName)
{
	for (uint32_t i = 0; i < m_shaderProgramParamNames.size(); ++i)
	{
		auto &p = m_shaderProgramParamNames[i];
		if (_stricmp(p.name, name) == 0 && (!parentName || _stricmp(p.parent, parentName) == 0))
		{
			return i;
		}
	}
	return uint32_t(-1);
}

void CShaderProgram::SetParams(ShaderProgramParams * params)
{
	ShaderProgramParam *p = params->pParams;
	for (uint32_t cnt = params->count; cnt; --cnt) {
		SetParam(p->modelId, p->paramId, p->pVal);
		++p;
	}
}

void CShaderProgram::SetParam(uint32_t objectId, uint32_t paramId, void * pVal)
{
	if (paramId < m_shaderProgramParamNames.size())
	{
		auto &p = m_shaderProgramParamNames[paramId];
		auto &buf = m_paramsBuffer[p.dataBufferId];
		uint8_t *data = buf.data() + p.stride * objectId + p.offset;
		memcpy_s(data, p.size, pVal, p.size);
	}
}
