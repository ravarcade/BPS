#pragma once

static const unsigned VULKAN_NUM_DESCRIPTOR_SETS = 4;
static const unsigned VULKAN_NUM_BINDINGS = 16;
const char * const SHAREDUBOTYPENAME = "SharedUBO";

struct VertexAttribDetails {
	uint32_t binding;
	uint32_t location;
	uint32_t vecsize;	// if vector it is size of vector ..... probably always = 1
	uint32_t type;		// see enum above
	VkFormat format;	// vulkan vertex format
	uint32_t size;		// 
	uint32_t offset;	// offset in binding
	Stream *pStream;      // stream in VertexDescription...
};

struct VertexAttribs {
	std::vector<VertexAttribDetails> attribs;
	std::vector<uint32_t> strides;
	uint32_t size;
};

struct ValMemberDetails {
	std::string name = "";
	std::string rootTypeName = "";
	uint32_t type = 0;     // int/float/...
	uint32_t vecsize = 0;  // num in row (1 - scaler, 2~4 vec2~vec4,...)
	uint32_t colsize = 0;  // matrix columns
	uint32_t matrix_stride = 0;
	uint32_t array = 0;
	uint32_t array_stride;
	uint32_t location = 0;
	uint32_t offset = 0;
	uint32_t size = 0;
	std::vector<ValMemberDetails> members;
	uint32_t propertyType = 0;
	uint32_t propertyCount = 0;
};

struct SampledImageDesc {
	uint32_t set = 0;
	uint32_t binding = 0;
	std::string name;
	uint32_t dim = 0;
	uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT;
};

struct ValDetails {
	uint32_t set = 0;
	uint32_t binding = 0;
	uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT;
	bool isSharedUBO = false;
	bool isHostVisibleUBO = false;
	ValMemberDetails entry;
};

class CShadersReflections
{
public:
	struct ShaderProgramInfo {
		BAMS::CResRawData resource;
		std::string entryPointName;
		VkShaderStageFlagBits stage;
	};

	struct ResourceLayout {
		struct DescriptorSetLayout {
			uint32_t uniform_buffer_mask = 0;
			uint32_t sampled_image_mask = 0;
			uint32_t descriptorCount[VULKAN_NUM_BINDINGS] = { 0 };
			uint32_t stages[VULKAN_NUM_BINDINGS] = { 0 };
		};

		DescriptorSetLayout descriptorSets[VULKAN_NUM_DESCRIPTOR_SETS];
		std::vector<VkPushConstantRange> pushConstants;
	};

	CShadersReflections();
	CShadersReflections(const char *shaderName);

	void LoadProgram(const char *shaderName);

	const std::vector<std::string> &GetOutputNames() { return m_outputNames; }

	const std::vector<ShaderProgramInfo> &GetPrograms() { return m_programs; }
	const ResourceLayout &GetLayout() { return m_layout; }
	const VertexAttribs &GetVertexAttribs() { return m_vertexAttribs; }
	const VertexDescription &GetVertexDescription() { return m_vertexDescription; }
	const std::vector<ValDetails> &GetParamsInUBO() { return m_params_in_ubos; }
	const std::vector<ValDetails> &GetParamsInPushConstants() { return m_params_in_push_constants; }
	const std::vector<SampledImageDesc> &GetSampledImages() { return m_sampled_images; }

	MProperties BuildProperties();

private:
	VertexDescription m_vertexDescription;

	VertexAttribs m_vertexAttribs;

	std::vector<ValDetails> m_params_in_ubos;
	std::vector<ValDetails> m_params_in_push_constants;
	std::vector<SampledImageDesc> m_sampled_images;
	uint32_t m_shared_ubos_size;
	uint32_t m_total_ubos_size;
	std::vector<uint32_t> m_ubo_sizes;

	std::vector<ShaderProgramInfo> m_programs;
	ResourceLayout m_layout;
	std::vector<std::string> m_outputNames;
	BAMS::CResShader m_shaderResource;

	bool m_enableAlpha;
	void _ParsePrograms();
};