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
	uint32_t vecsize;	// if vector it is size of vector ..... probably always = 1
	uint32_t type;		// see enum above
	VkFormat format;	// vulkan vertex format
	uint32_t size;		// 
	uint32_t offset;	// offset in binding
};

struct VertexAttribInfo {
	BAMS::RENDERINENGINE::VertexDescription descriptions;
	std::vector<VertexAttribDesc> attribs;
	std::vector<uint32_t> strides;
	uint32_t size;
};

union VertexDescriptionInfoPack {
	void *data;
	uint32_t info;
#pragma pack(push, 1)
	struct {
		uint16_t offset, binding;
	};
#pragma pack(pop)
};

enum ShaderReflectionType {
	UNKNOWN = 0,
	Int32 = 1, 
	UInt32 = 2,
	Int16 = 3,
	UInt16 = 4,
	Int8 = 5,
	UInt8 = 6,
	Float32 = 7,
};

struct ValMemberDetails {
	std::string name = "";
	std::string typenName = "";
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
	uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT;
	ValMemberDetails entry;

	std::vector<ValMemberDetails> members;
};

class CShadersReflections
{
public:
	struct ShaderProgramInfo {
		std::string name;
		::BAMS::CRawData resource;
		std::string entryPointName;
		VkShaderStageFlagBits stage;
		//	VkShaderModule shaderModule;
	};

	struct ResourceLayout {
		struct DescriptorSetLayout {
			uint32_t uniform_buffer_mask = 0;
			uint32_t descriptorCount[VULKAN_NUM_BINDINGS] = { 0 };
			uint32_t stages[VULKAN_NUM_BINDINGS] = { 0 };
		};

		DescriptorSetLayout descriptorSets[VULKAN_NUM_DESCRIPTOR_SETS];
		std::vector<VkPushConstantRange> pushConstants;
	};

	CShadersReflections();
	CShadersReflections(std::vector<std::string> &&programs);

	VertexAttribInfo LoadPrograms(std::vector<std::string> &&programs);
	void Release();

	const std::vector<std::string> &GetOutputNames() { return m_outputNames; }


	const std::vector<ShaderProgramInfo> &GetPrograms() { return m_programs; }
	const ResourceLayout &GetLayout() { return m_layout; }

private:
	VkPipelineVertexInputStateCreateInfo m_vertexInputInfo;

	VertexAttribInfo vi;

	std::vector<ValDetails> m_ubos;
	std::vector<ValDetails> m_push_constants;
	std::vector<ShaderProgramInfo> m_programs;
	ResourceLayout m_layout;

	bool m_enableAlpha;

	void _ParsePrograms();


	std::vector<std::string> m_outputNames;
};