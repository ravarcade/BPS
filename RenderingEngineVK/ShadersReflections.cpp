#include "stdafx.h"

#include "..\3rdParty\SPRIV-Cross\spirv_glsl.hpp"






void doTests()
{
//	ShadersReflections sr({"vert", "frag"});
//	sr.BuildVulkanStructures();
	return;
	BAMS::CResourceManager rm;
	auto code = rm.GetRawDataByName("vert");
	if (!code.IsLoaded())
		rm.LoadSync();

	spirv_cross::CompilerGLSL glsl(reinterpret_cast<const uint32_t*>(code.GetData()), (code.GetSize()+sizeof(uint32_t)-1)/sizeof(uint32_t));
	auto resources = glsl.get_shader_resources();
	for (auto &resource : resources.sampled_images)
	{
		unsigned set = glsl.get_decoration(resource.id, spv::DecorationDescriptorSet);
		unsigned binding = glsl.get_decoration(resource.id, spv::DecorationBinding);
		printf("Image %s at set = %u, binding = %u\n", resource.name.c_str(), set, binding);

		// Modify the decoration to prepare it for GLSL.
		glsl.unset_decoration(resource.id, spv::DecorationDescriptorSet);

		// Some arbitrary remapping if we want.
		glsl.set_decoration(resource.id, spv::DecorationBinding, set * 16 + binding);
	}

}

ShadersReflections::ShadersReflections()
{
}

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

	for (auto &prg : m_programs) 
	{		
		auto code = prg.resource;
		spirv_cross::CompilerGLSL compiler((uint32_t*)code.GetData(), (code.GetSize() + sizeof(uint32_t) - 1) / sizeof(uint32_t));
		auto resources = compiler.get_shader_resources();
		auto entry_points = compiler.get_entry_points_and_stages();
		auto &entry_point = entry_points[0];
		prg.entryPointName = entry_point.name;
		prg.stage = executionModelToStage[entry_point.execution_model];

		for (auto buffer : resources.uniform_buffers) {
			auto set = compiler.get_decoration(buffer.id, spv::DecorationDescriptorSet);
			auto binding = compiler.get_decoration(buffer.id, spv::DecorationBinding);
			auto name = compiler.get_name(buffer.id);
			spirv_cross::SPIRType type = compiler.get_type(buffer.type_id);
			uint32_t descriptorCount = type.array.size() == 1 ? type.array[0] : 1;

			m_layout.descriptorSets[set].uniform_buffer_mask |= 1u << binding;
			m_layout.descriptorSets[set].descriptorCount[binding] = descriptorCount;
			m_layout.descriptorSets[set].stages[binding] |= prg.stage;
		}

		if (prg.stage == VK_SHADER_STAGE_VERTEX_BIT) {
			for (auto attrib : resources.stage_inputs) {
				auto location = compiler.get_decoration(attrib.id, spv::DecorationLocation);
				m_layout.input_mask |= 1u << location;
			}
		}
	}
}

void ShadersReflections::BuildVkStructures(VkDevice device, const VkAllocationCallbacks* allocator)
{
	m_shaderStages.resize(m_programs.size());
	uint32_t idx = 0;

	for (auto &prg : m_programs) {
		auto code = prg.resource;

		VkShaderModuleCreateInfo createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
		createInfo.codeSize = code.GetSize();
		createInfo.pCode = reinterpret_cast<const uint32_t*>(code.GetData());
		if (vkCreateShaderModule(device, &createInfo, allocator, &prg.shaderModule) != VK_SUCCESS) {
			throw std::runtime_error("failed to create shader module!");
		}

		auto &stageCreateInfo = m_shaderStages[idx];
		stageCreateInfo = { VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO };
		stageCreateInfo.stage = prg.stage;
		stageCreateInfo.module = prg.shaderModule;
		stageCreateInfo.pName = prg.entryPointName.c_str();

		++idx;
	}

	for (auto &set : m_layout.descriptorSets) {
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		for (unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
		{
			uint32_t bitMask = 1u << i;
			uint32_t descriptorCount = set.descriptorCount[i];
			uint32_t stages = set.stages[i];

			if (set.uniform_buffer_mask & bitMask) {
				bindings.push_back({ i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount, stages, nullptr });
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
		// add uniform buffers
	//	Utils::for_each_bit(set.uniform_buffer_mask, [&](uint32_t bit) {
	//	});		
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
}

uint32_t ShadersReflections::AddDescriptorPoolsSize(std::vector<uint32_t>& poolsSize)
{
	if (poolsSize.size() < VK_DESCRIPTOR_TYPE_RANGE_SIZE) {
		size_t os = poolsSize.size();
		poolsSize.resize(VK_DESCRIPTOR_TYPE_RANGE_SIZE);
		for (uint32_t i = os; i < poolsSize.size(); ++i) {
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

