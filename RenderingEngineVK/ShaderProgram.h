
class OutputWindow; // parent output window class with working interface to Vulkan


struct ShaderProgramParam {
	uint32_t modelId;
	uint32_t paramId;
	void *pVal;
};

struct ShaderProgramParams {
	uint32_t count;
	ShaderProgramParam *pParams;
};

enum {
	PUSHCONSTANT = 1,
	UBO = 2
};

struct ShaderProgramParamDesc{
	uint32_t type;
	const char *name;
	const char *parent;
	uint32_t offset;
	uint32_t size;
	uint32_t stride;
	uint32_t dataBufferId;
};

struct ShaderProgramParamsDesc {
	uint32_t count;
	ShaderProgramParamDesc *pParams;
};

class CShaderProgram
{
public:
	CShaderProgram() = default;
	CShaderProgram(OutputWindow *outputWindow) : vk(outputWindow) {}

	void SetParentOutputWindow(OutputWindow *outputWindow) { vk = outputWindow; }

	void LoadPrograms(std::vector<std::string> &&programs);
	void Release();

	const std::vector<std::string> &GetOutputNames();
	const std::vector<VkDescriptorSetLayout> &GetDescriptorSetLayout() const { return m_descriptorSetLayout; }
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

	void SetRenderPassAndMsaaSamples(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);
	VkPipeline CreateGraphicsPipeline();
	void CreatePipelineLayout();

	void CreateModelBuffers(uint32_t numVertices, uint32_t numIndeces, uint32_t numObjects);
	uint32_t SendVertexData(BAMS::RENDERINENGINE::VertexDescription src);
	void SendVertexData(BAMS::RENDERINENGINE::VertexDescription src, void *dst);
	uint32_t GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize) { return _GetDescriptorPoolsSize(poolsSize); }
	void CreateDescriptorSets();
	void UpdateDescriptorSets();
	void DrawObject(VkCommandBuffer &cb, uint32_t objectId);

	uint32_t GetParamId(const char *name, const char *parentName = nullptr);
	void SetParams(ShaderProgramParams *params);
	void SetParam(uint32_t modelId, uint32_t paramId, void *pVal);
	const ShaderProgramParamsDesc * GetParams() { return &m_shaderProgramParamsDesc; }
	
private:
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
	uint32_t _GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize);
	void _SendVertexStream(BAMS::RENDERINENGINE::Stream dst, BAMS::RENDERINENGINE::Stream &src, uint8_t *outBuf, std::vector<uint32_t> &bindingOffset, uint32_t numVertices);
	void _CreateUniformBuffers();
	void _BuindShaderProgramParamsDesc();
	void _BuindShaderProgramParamsDesc(const std::vector<ValDetails> *vals, uint32_t &dataBufferId);
	void _BuindShaderProgramParamsDesc(const ValMemberDetails &entry, const char *parentName, uint32_t stride, uint32_t dataBufferId);
	void _BuindShaderDataBuffers();

	CShadersReflections          m_reflection;
	OutputWindow                 *vk                     = nullptr;

	VkRenderPass                 m_renderPass            = nullptr;
	VkSampleCountFlagBits        m_msaaSamples           = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineLayout             m_pipelineLayout        = nullptr;
	VkPipeline                   m_graphicsPipeline      = nullptr;
	VkBuffer                     m_vertexBuffer          = nullptr;
	VkDeviceMemory               m_vertexBufferMemory    = nullptr;
	VkBuffer                     m_indexBuffer           = nullptr;
	VkDeviceMemory               m_indexBufferMemory     = nullptr;
	std::vector<VkBuffer>        m_uniformBuffers;
	std::vector<VkDeviceMemory>  m_uniformBuffersMemory;

	uint32_t                     m_usedVerticesCount     = 0;
	uint32_t                     m_usedIndicesCount      = 0;
	VkDescriptorSet              m_descriptorSet		 = nullptr;
	std::vector<VkPipelineShaderStageCreateInfo>   m_shaderStages;
	std::vector<VkDescriptorSetLayout>             m_descriptorSetLayout;
	std::vector<VkVertexInputBindingDescription>   m_bindingDescription;		// <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;		// <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer
	ShaderDataInfo                                 m_vi;
	std::vector<std::vector<uint8_t>>              m_paramsBuffer;

	ShaderProgramParamsDesc                        m_shaderProgramParamsDesc;
	std::vector<ShaderProgramParamDesc>            m_shaderProgramParamNames;

	struct DrawObjectData {
		uint32_t setId;
		uint32_t firstVertex;
		uint32_t firstIndex;
		uint32_t numTriangles;
	};

	std::vector<DrawObjectData> m_drawObjectData;

};