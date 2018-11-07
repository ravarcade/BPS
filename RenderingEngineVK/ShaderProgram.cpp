#include "stdafx.h"

using namespace BAMS;
using namespace BAMS::RENDERINENGINE;

void CShaderProgram::LoadPrograms(std::vector<std::string>&& programs)
{
	// to do: add support for loading program description from manifest
	m_vi = m_reflection.LoadPrograms(std::move(programs));
}

void CShaderProgram::Release()
{
	vk->vkDestroy(m_vertexBuffer);
	vk->vkFree(m_vertexBufferMemory);
	vk->vkDestroy(m_indexBuffer);
	vk->vkFree(m_indexBufferMemory);

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
	VkPipelineViewportStateCreateInfo viewportState = { VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO };
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

void CShaderProgram::CreateModelBuffers(uint32_t numVertices, uint32_t numIndeces, uint32_t numObjects)
{
	vk->_CreateBuffer(
		numVertices * m_vi.size,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_vertexBuffer,
		m_vertexBufferMemory);

	vk->_CreateBuffer(
		numIndeces * sizeof(uint32_t),
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		m_indexBuffer, 
		m_indexBufferMemory);

	m_usedVerticesCount = 0;
	m_usedIndicesCount = 0;
}

void CShaderProgram::SendVertexData(VertexDescription src)
{
	uint32_t vboSize = src.m_numVertices * m_vi.size;
	/*
	std::vector<uint32_t> poolsSize;
	_GetDescriptorPoolsSize(poolsSize);
	for (auto ps : poolsSize)
		vboSize += src.m_numVertices * ps;
		*/
	uint32_t iboSize = src.m_numIndices * sizeof(uint32_t);

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->_CreateBuffer(
		vboSize + iboSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->device, stagingBufferMemory, 0, vboSize + iboSize, 0, &data);
	SendVertexData(src, data);
	vkUnmapMemory(vk->device, stagingBufferMemory);

	vk->_CopyBuffer(stagingBuffer, m_vertexBuffer, vboSize, 0);
	vk->_CopyBuffer(stagingBuffer, m_indexBuffer, iboSize, vboSize);

	vk->vkDestroy(stagingBuffer);
	vk->vkFree(stagingBufferMemory);
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

void CShaderProgram::SendVertexData(VertexDescription src, void *dst)
{
	uint32_t vboSize = 0;
	std::vector<uint32_t> bindingOffset;
	bindingOffset.push_back(0);

	for (auto ps : m_vi.strides) {
		vboSize += src.m_numVertices * ps;
		bindingOffset.push_back(vboSize);
	}

	uint32_t iboSize = src.m_numIndices * sizeof(uint32_t);

	CAttribStreamConverter conv;

//	auto &attribs = m_reflection.GetVertexAttribs();
	auto &d = m_vi.descriptions;
	auto outBuf = reinterpret_cast<uint8_t *>(dst);
#define SENDVERTEXSTREAM(x) _SendVertexStream(d.m_ ## x, src.m_ ## x, outBuf, bindingOffset, src.m_numVertices);
	SENDVERTEXSTREAM(vertices);
	SENDVERTEXSTREAM(colors[0]);
#undef SENDVERTEXSTREAM

	Stream out((U32)IDX_UINT32_1D, sizeof(uint32_t), false, reinterpret_cast<uint8_t *>(dst) + vboSize);
	conv.Convert(out, src.m_indices, src.m_numIndices);
}

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

void CShaderProgram::DrawObject(VkCommandBuffer & cb, uint32_t objectId)
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	static struct {
		glm::mat4 model;
		glm::vec4 baseColor;
	} pushConstantData;
	pushConstantData.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	pushConstantData.baseColor = glm::vec4(1, 0.5, 1, 1);

	if (m_vertexBuffer && m_indexBuffer) {
		VkBuffer vertexBuffers[] = { m_vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);

		vkCmdPushConstants(cb, m_pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0, 80, &pushConstantData);
		//	vkCmdBindIndexBuffer(cb, m_indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		vkCmdBindIndexBuffer(cb, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);

		vkCmdDrawIndexed(cb, 36, 1, 0, 0, 0);
	}
}
