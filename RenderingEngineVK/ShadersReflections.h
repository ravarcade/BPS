#pragma once


static const unsigned VULKAN_NUM_DESCRIPTOR_SETS = 4;
static const unsigned VULKAN_NUM_BINDINGS = 16;

struct ObjectMemoryRequirements
{
	uint32_t pushConstants;
	std::vector<uint32_t> ubos;
	uint32_t totalUbos;
	uint32_t maxUbos;
};

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
	std::vector<ValMemberDetails> members;
};

struct ValDetails {
	uint32_t set = 0;
	uint32_t binding = 0;
	uint32_t stage = VK_SHADER_STAGE_VERTEX_BIT;
	ValMemberDetails entry;
};

struct ShaderDataInfo {
	BAMS::RENDERINENGINE::VertexDescription descriptions;
	std::vector<VertexAttribDesc> attribs;
	std::vector<uint32_t> strides;
	uint32_t size;

	std::vector<ValDetails> params_in_ubos;
	std::vector<ValDetails> params_in_push_constants;
	uint32_t push_constatns_size;
	uint32_t shared_ubos_size;
	uint32_t total_ubos_size;
	uint32_t max_single_ubo_size;
	std::vector<uint32_t> ubo_sizes;
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
	Vec2 = 8,
	Vec3 = 9,
	Vec4 = 10,
	Mat3 = 11,
	Mat4 = 12
};

class CShadersReflections
{
public:
	struct ShaderProgramInfo {
		BAMS::CRawData resource;
		std::string entryPointName;
		VkShaderStageFlagBits stage;
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
	CShadersReflections(const char *shaderName);

	ShaderDataInfo LoadProgram(const char *shaderName);
	void Release();

	const std::vector<std::string> &GetOutputNames() { return m_outputNames; }


	const std::vector<ShaderProgramInfo> &GetPrograms() { return m_programs; }
	const ResourceLayout &GetLayout() { return m_layout; }

private:
	ShaderDataInfo vi;

	std::vector<ShaderProgramInfo> m_programs;
	ResourceLayout m_layout;

	bool m_enableAlpha;

	void _ParsePrograms();


	std::vector<std::string> m_outputNames;
	BAMS::CShader m_shaderResource;
};