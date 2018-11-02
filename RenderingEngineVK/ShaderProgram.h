
class OutputWindow;

struct CSharedWithShaderProgram
{
	uint32_t graphicsFamily;
	uint32_t transferFamily;
	VkQueue graphicsQueue;
	VkQueue transferQueue;
	VkCommandPool commandPool;
	VkCommandPool transferPool;
	VkDevice logicalDevice;
	const VkAllocationCallbacks* memAllocator;
	VkRenderPass renderPass;
	VkSampleCountFlagBits msaaSamples;
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
	void SendVertexData(BAMS::RENDERINENGINE::VertexDescription src);
	void SendVertexData(BAMS::RENDERINENGINE::VertexDescription src, void *dst);
	uint32_t GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize) { return _GetDescriptorPoolsSize(poolsSize); }
	void DrawObject(VkCommandBuffer &cb, uint32_t objectId);

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

	CShadersReflections          m_reflection;
	OutputWindow                 *vk;

	VkRenderPass                 m_renderPass            = nullptr;
	VkSampleCountFlagBits        m_msaaSamples           = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineLayout             m_pipelineLayout        = nullptr;
	VkPipeline                   m_graphicsPipeline      = nullptr;
	VkBuffer                     m_vertexBuffer          = nullptr;
	VkDeviceMemory               m_vertexBufferMemory    = nullptr;
	VkBuffer                     m_indexBuffer           = nullptr;
	VkDeviceMemory               m_indexBufferMemory     = nullptr;
	VkBuffer                     m_uniformBuffer         = nullptr;
	VkDeviceMemory               m_uniformBufferMemory   = nullptr;

	uint32_t                     m_usedVerticesCount     = 0;
	uint32_t                     m_usedIndicesCount      = 0;

	std::vector<VkPipelineShaderStageCreateInfo>   m_shaderStages;
	std::vector<VkDescriptorSetLayout>             m_descriptorSetLayout;
	std::vector<VkVertexInputBindingDescription>   m_bindingDescription;		// <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;		// <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer
	VertexAttribInfo                               m_vi;
};