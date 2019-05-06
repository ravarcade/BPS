#include "stdafx.h"

using namespace BAMS;
using namespace BAMS::RENDERINENGINE;
using BAMS::CORE::Property;

const uint32_t ALWAYS_RESERVE_VERTICES = 16 * 1024; // <- 256kB for 16bytes / per vertice
const uint32_t ALWAYS_RESERVE_INDICES = ALWAYS_RESERVE_VERTICES;

/// <summary>
/// Loads the program.
/// Will not call any vulkan function or create any vulkan object.
/// It will only parse program.
/// To create vk object you need to call other methods, like CreateGraphicsPipeline()
/// </summary>
/// <param name="program">The program.</param>
void CShaderProgram::LoadProgram(const char *program)
{
	// to do: add support for loading program description from manifest
	m_reflection.LoadProgram(program);
	_BuildProperties();
}

void CShaderProgram::Release()
{
	for (auto &ubo : m_uniformBuffers)
	{
		// we don't want to destroy shared ubo
		if (!ubo.isSharedUbo)
			vk->vkDestroy(ubo);
	}
	m_uniformBuffers.clear();

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

void CShaderProgram::CreateGraphicsPipeline()
{
	vk->vkDestroy(m_graphicsPipeline); // delete graphicsPipline if it exist

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

#ifdef _DEBUG
	for (uint32_t i = 0; i < pipelineInfo.pVertexInputState->vertexAttributeDescriptionCount; ++i)
	{
		if (!vk->IsBufferFeatureSupported(pipelineInfo.pVertexInputState->pVertexAttributeDescriptions[i].format, VK_FORMAT_FEATURE_VERTEX_BUFFER_BIT))
		{
			throw std::runtime_error("Failed to create graphics pipeline! Attrib vertex fromat is not supported.");
		}
	}
#endif

	if (vkCreateGraphicsPipelines(vk->device, VK_NULL_HANDLE, 1, &pipelineInfo, vk->allocator, &m_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("Failed to create graphics pipeline!");
	}
	// some cleanups
	for (auto shaderStage : shaderStages)
		vk->vkDestroy(shaderStage.module);

	// other stuffs use by shader program
	_CreateUniformBuffers();
	_BuildShaderDataBuffers();
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
	auto &va = _GetVertexAttribs();

	for (uint32_t binding = 0; binding < va.strides.size(); ++binding)
	{
		auto stride = va.strides[binding];
		if (stride > 0)
		{
			m_bindingDescription.push_back({
				binding,
				stride,
				VK_VERTEX_INPUT_RATE_VERTEX
			});
		}
	}

	for (auto &attr : va.attribs)
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

	static VkPipelineColorBlendAttachmentState noBlending;
	noBlending.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	noBlending.blendEnable = VK_FALSE;

	noBlending.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	noBlending.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	noBlending.colorBlendOp = VK_BLEND_OP_ADD; // Optional

	noBlending.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	noBlending.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	noBlending.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

	auto &outputNames = m_reflection.GetOutputNames();
	for (auto &name : outputNames)
	{
		colorBlendAttachments.push_back(noBlending);
	}

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

VkRenderPass CShaderProgram::_GetRenderPass()
{
	return m_renderPass;
}

uint32_t CShaderProgram::_GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize)
{
	if (poolsSize.size() < VK_DESCRIPTOR_TYPE_RANGE_SIZE) 
	{
		uint32_t i = static_cast<uint32_t>(poolsSize.size());
		poolsSize.resize(VK_DESCRIPTOR_TYPE_RANGE_SIZE);
		for (; i < poolsSize.size(); ++i) {
			poolsSize[i] = 0;
		}
	}

	auto layout = m_reflection.GetLayout();

	for (auto &set : layout.descriptorSets) 
	{
		if (set.uniform_buffer_mask)
			poolsSize[VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER] += Utils::count_bits(set.uniform_buffer_mask);
		
		if (set.sampled_image_mask) 
			poolsSize[VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER] += Utils::count_bits(set.sampled_image_mask);
		
	}

	uint32_t nonZero = 0;
	for (auto x : poolsSize) {
		if (x > 0)
			++nonZero;
	}

	return nonZero;
}

VkPipelineLayout CShaderProgram::_GetPipelineLayout()
{
	if (m_pipelineLayout != VK_NULL_HANDLE)
		return m_pipelineLayout;

	m_descriptorSetLayout.clear();

	uint32_t lastSet = 0, i = 0;
	auto layout = m_reflection.GetLayout();
	for (auto &set : layout.descriptorSets)
	{
		if (set.uniform_buffer_mask ||
			set.sampled_image_mask)
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

		if (set.sampled_image_mask)
		{
			for (unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
			{
				uint32_t bitMask = 1u << i;
				uint32_t descriptorCount = set.descriptorCount[i];
				uint32_t stages = set.stages[i];

				if (set.sampled_image_mask & bitMask) {
					bindings.push_back({ i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount, stages, nullptr });
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
	return m_pipelineLayout;
}

void CShaderProgram::_CreateNewBufferSet(uint32_t numVertices, uint32_t numIndeces)
{
	auto &va = _GetVertexAttribs();
	if (!va.size) // we don't create buffers if we don't have any vertex input
		return;

	BufferSet set;
	vk->_CreateBuffer(
		numVertices * va.size,
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

VertexDescription *CShaderProgram::_GetMeshVertexDescription(const char *name)
{
	// TODO: Load mesh from resources
	static VertexDescription vd;
	BAMS::CResourceManager rm;
	if (auto res = rm.Find(name, RESID_MESH))
	{
		CMesh m(res);
		auto pvd = reinterpret_cast<VertexDescription*>(m.GetVertexDescription());
		if (pvd)
		{
			vd = *pvd;
			return &vd;
		}
	}
	return nullptr;
}

uint32_t CShaderProgram::AddMesh(const char *name)
{
	if (auto pMeshId = m_meshNames.find(name))
		return *pMeshId;

	auto &va = _GetVertexAttribs();

	// if this shader don't use any geometry (like deferred resolve shader) we don'y need to load any mesh
	if (!va.size)
	{
		Mesh m = { 0, 0, 0, 0, 3 }; // empty mesh with 3 indices (they are not readed from mem, they are computed in shader)

		uint32_t meshId = static_cast<uint32_t>(m_meshes.size());
		m_meshes.push_back(m);
		*m_meshNames.add(name) = meshId;
		return meshId;
	}

	const VertexDescription *vd = _GetMeshVertexDescription(name);
	if (!vd)
		return -1;

	// we check only last BufferSet
	BufferSet *set = m_bufferSets.size() ? &*m_bufferSets.rbegin() : nullptr;
	if (set == nullptr || set->freeVertices < vd->m_numVertices || set->freeIndices < vd->m_numIndices)
	{
		_CreateNewBufferSet(
			vd->m_numVertices < ALWAYS_RESERVE_VERTICES ? vd->m_numVertices + ALWAYS_RESERVE_VERTICES : vd->m_numVertices,
			vd->m_numIndices < ALWAYS_RESERVE_INDICES ? vd->m_numIndices + ALWAYS_RESERVE_INDICES : vd->m_numIndices);
		set = &*m_bufferSets.rbegin();
	}
	uint32_t bufferSetIdx = static_cast<uint32_t>(m_bufferSets.size()) - 1;

	uint32_t indeceSize = sizeof(uint32_t);
	uint32_t firstIndex = set->usedIndices;
	uint32_t vboSize = vd->m_numVertices * va.size;
	uint32_t iboSize = vd->m_numIndices * indeceSize;

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	vk->_CreateBuffer(
		vboSize + iboSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(vk->device, stagingBufferMemory, 0, vboSize + iboSize, 0, &data);
	_ImportMeshData(vd, data);
	vkUnmapMemory(vk->device, stagingBufferMemory);

	vk->_CopyBuffer(stagingBuffer, set->vertexBuffer, vboSize, 0, set->usedVertices * va.size);
	vk->_CopyBuffer(stagingBuffer, set->indexBuffer, iboSize, vboSize, set->usedIndices * indeceSize);

	vk->vkDestroy(stagingBuffer);
	vk->vkFree(stagingBufferMemory);

	// add "mesh"
	Mesh m = {
		bufferSetIdx,
		set->usedVertices,
		set->usedIndices,
		vd->m_numVertices,
		vd->m_numIndices
	};

	set->usedVertices += vd->m_numVertices;
	set->usedIndices += vd->m_numIndices;

	uint32_t meshId = static_cast<uint32_t>(m_meshes.size());
	m_meshes.push_back(m);

	*m_meshNames.add(name) = meshId;

	return meshId;
}

uint32_t CShaderProgram::AddObject(uint32_t meshIdx)
{
	DrawObjectData dod;
	dod.meshIdx = meshIdx;
	dod.paramsSetId = static_cast<uint32_t>(m_drawObjectData.size());
	m_drawObjectData.push_back(dod);

	return dod.paramsSetId;
}

void CShaderProgram::_CreateUniformBuffers()
{
	auto &uboParams = m_reflection.GetParamsInUBO();
	for (auto &ubo : uboParams)
	{
		if (ubo.isSharedUBO) // "ubo" is global/shared ubo and we do not create it here... only separate ones used in this shader
		{
			m_uniformBuffers.push_back({
				ubo.binding,
				ubo.stage,
				ubo.entry.size,
				vk->sharedUniformBuffer,
				vk->sharedUniformBufferMemory,
				vk->sharedUboData,
				true});
			continue;
		}

		// TODO:
		// Allocate buffers to store give number of objects, there is no reason to have all buffers same size
		// Right now we allocate space for only one object in UBO. All is passed with push_constants.

		uint32_t numObjects = 1;
		VkDeviceSize bufferSize = ubo.entry.size * numObjects;
		VkBuffer buf = VK_NULL_HANDLE;
		VkDeviceMemory mem = VK_NULL_HANDLE;
		VkMemoryPropertyFlags memProp = ubo.isHostVisibleUBO ? VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT : VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
		void *mappedBuffer = nullptr;

		vk->_CreateBuffer(
			bufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT,
			memProp,
			buf, mem);
		
		if (ubo.isHostVisibleUBO)
		{
			vkMapMemory(vk->device, mem, 0, bufferSize, 0, &mappedBuffer);
		}

		m_uniformBuffers.push_back({
			ubo.binding, 
			ubo.stage, 
			static_cast<uint32_t>(bufferSize), 
			buf,
			mem,
			mappedBuffer,
			false});

	}
}

/// <summary>
/// Builds the shader properties.
/// For every ubo binding, we will have separete buffer, also one for push constants.
/// </summary>
void CShaderProgram::_BuildProperties()
{
	_BuindShaderProgramParamsDesc();
	// we want to skip "SharedUBO"
	m_properties.clear();


	// add all parents for lev 0
	uint32_t paramIdx = -1;
	for (auto &p : m_shaderProgramParamNames)
	{
		++paramIdx;
		if (p.root->isSharedUBO)
			continue;

		p.propertyParentIdx = paramIdx;
		uint32_t parentId = -1;
		if (p.parentIdx < m_shaderProgramParamNames.size())
		{
			parentId = m_shaderProgramParamNames[p.parentIdx].propertyParentIdx;
		}

		// add entry
		auto pr = m_properties.add();
		pr->name = p.mem->name.c_str();
		pr->parent = parentId;
		pr->idx = paramIdx;
		pr->type = p.mem->propertyType;
		pr->count = p.mem->propertyCount;
		pr->array_stride = p.mem->array_stride;
	}
}

void CShaderProgram::_BuindShaderProgramParamsDesc()
{
	m_shaderProgramParamNames.clear();

	uint32_t dataBufferId = 0;
	auto &pcParams = m_reflection.GetParamsInPushConstants();
	auto &uboParams = m_reflection.GetParamsInUBO();
	_BuindShaderProgramParamsDesc(&pcParams, dataBufferId);
	_BuindShaderProgramParamsDesc(&uboParams, dataBufferId);

	m_isPushConstantsUsed = pcParams.size() && m_shaderProgramParamNames.size();
}

void CShaderProgram::_BuindShaderProgramParamsDesc(const std::vector<ValDetails>* vals, uint32_t &dataBufferId)
{
	for (auto &vd : *vals) {
		_BuindShaderProgramParamsDesc(vd.entry, -1, vd, dataBufferId);
		++dataBufferId;
	}
}

void CShaderProgram::_BuindShaderProgramParamsDesc(const ValMemberDetails &entry, uint32_t parentIdx, const ValDetails &root, uint32_t dataBufferId)
{
	assert(entry.members.size() > 0);

	for (auto &mem : entry.members)
	{
		if (mem.members.size())
		{
			// add parent?
			if (mem.propertyType == Property::PT_ARRAY) // it is array! add parrent
			{
				m_shaderProgramParamNames.push_back({
					&root,
					&mem,
					parentIdx,
					dataBufferId,
					static_cast<uint32_t>(-1)
				});
				parentIdx = static_cast<uint32_t>(m_shaderProgramParamNames.size() - 1);
			}
			_BuindShaderProgramParamsDesc(mem, parentIdx, root, dataBufferId);
		}
		else if (mem.propertyType != BAMS::CORE::Property::PT_UNKNOWN)
		{
			m_shaderProgramParamNames.push_back({
				&root,
				&mem,
				parentIdx,
				dataBufferId,
				static_cast<uint32_t>(-1)
			});
		}
	}
}

const uint32_t MAXBUFFERCHUNKSIZE = 32 * 1024;
const uint32_t MAXNUMOBJCTS = 1000;

void CShaderProgram::_BuildShaderDataBuffers()
{
	auto &pcParams = m_reflection.GetParamsInPushConstants();
	auto &uboParams = m_reflection.GetParamsInUBO(); 
	// I can't use m_uniformBuffers :( It is not initialized yet. 
	// I have to make all decision base on uboParams
	
	// calc required memory to store own data
	uint32_t localMemorySize = 0;
	for (auto &pc : pcParams)
		localMemorySize += pc.entry.size;

	for (auto &ub : m_uniformBuffers)
		if (!ub.isSharedUbo && ub.mappedBuffer)
			localMemorySize += ub.size;

	uint32_t maxObjects = MAXNUMOBJCTS;
	if (localMemorySize)
		maxObjects = MAXBUFFERCHUNKSIZE / localMemorySize;

	if (maxObjects > MAXNUMOBJCTS)
		maxObjects = MAXNUMOBJCTS;

	m_paramsLocalBuffer.resize(localMemorySize*maxObjects);
	auto localBuf = m_paramsLocalBuffer.data();
	
	m_paramsBuffers.clear();
	for (auto &pc : pcParams)
	{
		m_paramsBuffers.push_back({
			PUSH_CONSTANTS,
			localBuf,
			pc.entry.size,
			false
			});
		localBuf += pc.entry.size * maxObjects;
	}

	for (auto &ub : m_uniformBuffers)
	{
		if (ub.mappedBuffer)
		{
			m_paramsBuffers.push_back({
				HOSTVISIBLE_UBO,
				reinterpret_cast<uint8_t*>(ub.mappedBuffer),
				ub.size,
				false
				});

		}
		else
		{
			m_paramsBuffers.push_back({
				UBO,
				localBuf,
				ub.size,
				false
				});

			localBuf += ub.size * maxObjects;
		}
	}	
}

void CShaderProgram::_ImportMeshData(const VertexDescription *srcVD, void *dstBuf)
{
	auto va = _GetVertexAttribs(); // copy of VertexAttribs. We don't want to overwrite original

	auto outBuf = reinterpret_cast<uint8_t *>(dstBuf);
	uint32_t vboSize = 0; 
	uint32_t iboSize = srcVD->m_numIndices * sizeof(uint32_t);

	std::vector<uint32_t> bindingOffset;
	bindingOffset.push_back(0);
	
	for (auto ps : va.strides) {
		vboSize += srcVD->m_numVertices * ps;
		bindingOffset.push_back(vboSize);
	}
	// buffer for different binds are put one after another
	// this is BAD, because different meshes can't share buffers...
	assert(vboSize == srcVD->m_numVertices * va.size);
	auto vd = m_reflection.GetVertexDescription();  // copy of Vertex Description. We don't want to overwrite original
	for (auto &attr : va.attribs)
	{
		auto s = vd.GetStream(attr.type);
		attr.pStream = s;
		s->m_data = outBuf + bindingOffset[attr.binding] + attr.offset;
	}
	vd.m_indices = Stream(VAT_IDX_UINT32_1D, sizeof(uint32_t), false, outBuf + vboSize);
	vd.Copy(*srcVD);
	vd.Dump();
}

void CShaderProgram::CreateDescriptorSets()
{
	m_descriptorSet = vk->CreateDescriptorSets(m_descriptorSetLayout);
}

void CShaderProgram::UpdateDescriptorSets()
{

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

	std::vector<VkWriteDescriptorSet> descriptorWrites;
	VkWriteDescriptorSet wds = {};
	wds.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	wds.dstSet = m_descriptorSet;
	wds.dstBinding = 0;
	wds.dstArrayElement = 0;

	// add sampled_images
	auto &sampledImages = m_reflection.GetSampledImages();
	for (auto &si : sampledImages)
	{
		wds.dstBinding = si.binding;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		wds.descriptorCount = 1;
		auto imageInfo = vk->GetDescriptionImageInfo(si.name.c_str());
		assert(imageInfo);
		wds.pImageInfo = imageInfo;
		descriptorWrites.push_back(wds);
	}

	// add UBO's
	std::vector<VkDescriptorBufferInfo> buffersInfo;
	buffersInfo.resize(m_uniformBuffers.size());
	VkDescriptorBufferInfo *pBufferInfo = buffersInfo.data();
	for (auto &ubo : m_uniformBuffers)
	{
		*pBufferInfo = {ubo.buffer, 0, ubo.size};

		wds.dstBinding = ubo.binding;
		wds.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		wds.descriptorCount = 1;
		wds.pBufferInfo = pBufferInfo;

		descriptorWrites.push_back(wds);
	}

	vkUpdateDescriptorSets(vk->device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

void CShaderProgram::DrawObjects(VkCommandBuffer & cb)
{
	vkCmdBindPipeline(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);

	uint32_t lastBufferSetIdx = -1;
	if (vk->currentDescriptorSet != m_descriptorSet)
	{
		vk->currentDescriptorSet = m_descriptorSet;
		vkCmdBindDescriptorSets(cb, VK_PIPELINE_BIND_POINT_GRAPHICS, m_pipelineLayout, 0, 1, &vk->currentDescriptorSet, 0, nullptr);
	}

//	TRACE("[" << m_drawObjectData.size() << "]");

	uint8_t *pcBuffer = nullptr;
	uint32_t pcStride = 0;
	uint32_t pcStage = 0;
	if (m_isPushConstantsUsed)
	{
		uint32_t pcBufferId = m_shaderProgramParamNames[0].dataBufferId;
		pcBuffer = m_paramsBuffers[pcBufferId].buffer;
		pcStride = m_paramsBuffers[pcBufferId].size;
		pcStage = m_shaderProgramParamNames[0].root->stage;
	}
	for (auto &dod : m_drawObjectData)
	{
		auto &m = m_meshes[dod.meshIdx];
		if (m.numVertex > 0 && m.bufferSetIdx != lastBufferSetIdx)
		{
			lastBufferSetIdx = m.bufferSetIdx;
			auto &bs = m_bufferSets[m.bufferSetIdx];
			VkBuffer vertexBuffers[] = { bs.vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(cb, 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(cb, bs.indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}

		// TODO:
		if (m_isPushConstantsUsed)
		{
			vkCmdPushConstants(cb, m_pipelineLayout, pcStage, 0, pcStride, pcBuffer);
			pcBuffer += pcStride;
		}
		if (m.numVertex == 0)
		{
			vkCmdDraw(cb, 3, 1, 0, 0);
		}
		else
		{
			vkCmdDrawIndexed(cb, m.numIndex, 1, m.firstIndex, m.firstVertex, 0);
		}
	}
}
