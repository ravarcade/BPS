#pragma once


static const unsigned VULKAN_NUM_DESCRIPTOR_SETS = 4;
static const unsigned VULKAN_NUM_BINDINGS = 16;



struct VertexAttribDesc {
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

	uint32_t binding;
	uint32_t location;
	uint32_t vecsize;
	uint32_t type;
	VkFormat format;
	uint32_t size;
	uint32_t offset;
};

enum ShaderReflectionType {
	UNKNOWN,
	Int32, UInt32, Float32
};

struct ValMemberDetails {
	std::string name = "";
	uint32_t type = 0;     // int/float
	uint32_t vecsize = 0;  // num in row (1 - scaler, 2~4 vec2~vec4,...)
	uint32_t colsize = 0;  // matrix columns
	uint32_t matrix_stride = 0;
	uint32_t array = 0;
	uint32_t array_stride;
	uint32_t location = 0;
	uint32_t offset = 0;
	uint32_t size = 0;
};

struct ValDetails {
	uint32_t set = 0;
	uint32_t binding = 0;
	ValMemberDetails entry;
	std::vector<ValMemberDetails> members;
};

class ShadersReflections
{
public:
	ShadersReflections();
	ShadersReflections(std::vector<std::string> &&programs);

	void LoadPrograms(std::vector<std::string> &&programs);
	void ReleaseVk(VkDevice device, const VkAllocationCallbacks* allocator);
	
	const std::vector<VkDescriptorSetLayout> &GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
	const std::vector<VkPipelineShaderStageCreateInfo> &GetShaderStages() const { return m_shaderStages; }
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }
	uint32_t AddDescriptorPoolsSize(std::vector<uint32_t> &poolsSize);

	VkPipelineVertexInputStateCreateInfo *GetVertexInputInfo();
	void CreatePipelineLayout(VkDevice device, const VkAllocationCallbacks* allocator);
	VkPipeline CreateGraphicsPipeline(VkDevice device, const VkAllocationCallbacks* allocator);

	void SetRenderPassAndMsaaSamples(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples) { m_renderPass = renderPass; m_msaaSamples = msaaSamples; }
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

	std::vector<VertexAttribDesc> m_VertexAttribs;
	VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;

	std::vector<ValDetails> m_ubos;
	std::vector<ShaderProgramInfo> m_programs;
	ResourceLayout m_layout;

	std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;
	std::vector<VkDescriptorSetLayout> m_descriptorSetLayout;
	bool m_enableAlpha;

	VkDevice logicalDevice = nullptr;
	const VkAllocationCallbacks* memAllocator = nullptr;
	VkRenderPass m_renderPass;
	VkSampleCountFlagBits m_msaaSamples;
	VkPipelineLayout m_pipelineLayout;
	VkPipeline m_graphicsPipeline;

	void ParsePrograms();

	std::vector<VkPipelineShaderStageCreateInfo> _Compile();
	VkPipelineVertexInputStateCreateInfo _GetVertexInputInfo();
	VkPipelineInputAssemblyStateCreateInfo _GetInputAssembly();
	VkPipelineViewportStateCreateInfo  _GetViewportState();
	VkPipelineRasterizationStateCreateInfo _GetRasterizationState();
	VkPipelineMultisampleStateCreateInfo _GetMultisampleState();
	VkPipelineDepthStencilStateCreateInfo _GetDepthStencilState();
	VkPipelineColorBlendStateCreateInfo _GetColorBlendState();
	VkPipelineDynamicStateCreateInfo _GetDynamicState();
	VkPipelineLayout _GetPipelineLayout();
	VkRenderPass _GetRenderPass();

	std::vector<VkVertexInputBindingDescription> m_bindingDescription;      // <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions; // <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer
};