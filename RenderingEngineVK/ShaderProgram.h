
class OutputWindow; // parent output window class with working interface to Vulkan


struct ShaderProgramParam {
	uint32_t objectId;
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

	void LoadProgram(const char *program);
	void Release();

	const std::vector<std::string> &GetOutputNames();
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

	void SetRenderPassAndMsaaSamples(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples);
	VkPipeline CreateGraphicsPipeline();
	void CreatePipelineLayout();

	uint32_t AddMesh(const char *name);
	uint32_t AddObject(uint32_t meshIdx);

	uint32_t GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize) { return _GetDescriptorPoolsSize(poolsSize); }
	void CreateDescriptorSets();
	void UpdateDescriptorSets();
	void DrawObjects(VkCommandBuffer &cb);

	uint32_t GetParamId(const char *name, const char *parentName = nullptr);
	void SetParams(ShaderProgramParams *params);
	void SetParam(uint32_t objectId, uint32_t paramId, void *pVal);
	const ShaderProgramParamsDesc * GetParams() { return &m_shaderProgramParamsDesc; }
	BAMS::CORE::Properties *GetProperties(uint32_t drawObjectId = -1) {
		if (drawObjectId != -1)
		{
			for (auto &p : m_properties)
			{
				if (p.type != BAMS::CORE::Property::PT_EMPTY)
				{
					auto pb = m_shaderProgramParamNames[p.idx];
					auto &buf = m_paramsBuffer[pb.dataBufferId];
					p.val = buf.data() + pb.stride * drawObjectId + pb.offset;
				}
			}
		}
		return &m_properties; 
	}
	uint32_t GetObjectCount() { return static_cast<uint32_t>(m_drawObjectData.size()); }

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
	void _CreateUniformBuffers();
	void _BuildShaderProperties();
	void _BuindShaderProgramParamsDesc();
	void _BuindShaderProgramParamsDesc(const std::vector<ValDetails> *vals, uint32_t &dataBufferId);
	void _BuindShaderProgramParamsDesc(const ValMemberDetails &entry, const char *parentName, uint32_t stride, uint32_t dataBufferId);
	void _BuindShaderDataBuffers();

	void _CreateNewBufferSet(uint32_t numVertices, uint32_t numIndeces);

	BAMS::RENDERINENGINE::VertexDescription * _GetMeshVertexDescription(const char * name);
	void _ImportMeshData(const BAMS::RENDERINENGINE::VertexDescription *vd, void *dst);

	CShadersReflections          m_reflection;
	OutputWindow                 *vk                     = nullptr;

	VkRenderPass                 m_renderPass            = nullptr;
	VkSampleCountFlagBits        m_msaaSamples           = VK_SAMPLE_COUNT_1_BIT;

	VkPipelineLayout             m_pipelineLayout        = nullptr;
	VkPipeline                   m_graphicsPipeline      = nullptr;

	std::vector<VkDescriptorSetLayout>             m_descriptorSetLayout;

	std::vector<VkVertexInputBindingDescription>   m_bindingDescription;		// <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;		// <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer
	ShaderDataInfo                                 m_vi;
	std::vector<std::vector<uint8_t>>              m_paramsBuffer;

	BAMS::CORE::MProperties                        m_properties;
	ShaderProgramParamsDesc                        m_shaderProgramParamsDesc;
	std::vector<ShaderProgramParamDesc>            m_shaderProgramParamNames;
	uint32_t                                       m_pushConstantsStride;
	uint32_t                                       m_pushConstantsBufferId;
	uint32_t                                       m_pushConstantsStages;


	VkDescriptorSet              m_descriptorSet = nullptr;
	std::vector<VkBuffer>        m_uniformBuffers;
	std::vector<VkDeviceMemory>  m_uniformBuffersMemory;

	struct BufferSet {
		VkBuffer       vertexBuffer;
		VkDeviceMemory vertexBufferMemory;
		VkBuffer       indexBuffer;
		VkDeviceMemory indexBufferMemory;
		uint32_t       usedVertices;
		uint32_t       freeVertices;
		uint32_t       usedIndices;
		uint32_t       freeIndices;
	};

	struct Mesh {
		uint32_t bufferSetIdx;
		uint32_t firstVertex;
		uint32_t firstIndex;
		uint32_t numVertex;
		uint32_t numIndex;
	};

	struct DrawObjectData {
		uint32_t paramsSetId;
		uint32_t meshIdx;
	};

	std::vector<BufferSet> m_bufferSets;
	std::vector<Mesh> m_meshes;
	std::vector<DrawObjectData> m_drawObjectData;
	BAMS::CORE::hashtable<const char *, uint32_t> m_meshNames;

};