#pragma once


static const unsigned VULKAN_NUM_DESCRIPTOR_SETS = 4;
static const unsigned VULKAN_NUM_BINDINGS = 16;


class ShadersReflections
{
public:
	ShadersReflections();
	ShadersReflections(std::vector<std::string> &&programs);

	void LoadPrograms(std::vector<std::string> &&programs);
	void BuildVkStructures(VkDevice device, const VkAllocationCallbacks* allocator);
	void ReleaseVk(VkDevice device, const VkAllocationCallbacks* allocator);
	
	const std::vector<VkDescriptorSetLayout> &GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
	const std::vector<VkPipelineShaderStageCreateInfo> &GetShaderStages() const { return m_shaderStages; }
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
	uint32_t AddDescriptorPoolsSize(std::vector<uint32_t> &poolsSize);
private:
	struct ResourceLayout {
		struct DescriptorSetLayout {
			uint32_t uniform_buffer_mask = 0;
			uint32_t descriptorCount[VULKAN_NUM_BINDINGS] = { 0 };
			uint32_t stages[VULKAN_NUM_BINDINGS] = { 0 };
		};
		uint32_t input_mask = 0;
//		uint32_t output_mask = 0;
		uint32_t push_constant_offset = 0;
		uint32_t push_constant_range = 0;
		uint32_t spec_constant_mask = 0;
		DescriptorSetLayout descriptorSets[VULKAN_NUM_DESCRIPTOR_SETS];
	};

	struct ShaderProgramInfo {
		std::string name;
		::BAMS::CRawData resource;
		std::string entryPointName;
		VkShaderStageFlagBits stage;
		VkShaderModule shaderModule;
	};

	std::vector<ShaderProgramInfo> m_programs;
	ResourceLayout m_layout;
	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayout;
	VkPipelineLayout m_pipelineLayout;

	void ParsePrograms();
};