
class OutputWindow; // parent output window class with working interface to Vulkan


/// <summary>
/// To set any param for shader program (regardless it is in push constants or ubo), you need:
/// objectId - index of object drawed by this shader. [uint32_t]
/// paramId - index of param for this shader. [uint32_t]
/// pVal - pointer to memory with value of param. [void *]
/// </summary>
struct ShaderProgramParam {
	uint32_t objectId;
	uint32_t paramId;
	void *pVal;
};

struct UniformBuffer
{
	uint32_t binding;
	uint32_t stage;
	uint32_t size;
	VkBuffer buffer;
	VkDeviceMemory memory;	
	void *mappedBuffer;
	bool isSharedUbo;
};

class CShaderProgram
{
	struct ShaderProgramParamDesc {
		const ValDetails *root;
		const ValMemberDetails *mem;
		uint32_t parentIdx;
		uint32_t dataBufferId;
		uint32_t propertyParentIdx;
	};

public:
	CShaderProgram() = default;
	CShaderProgram(OutputWindow *outputWindow) : vk(outputWindow) {}

	void LoadProgram(const char *program);
	void Release();

	const std::vector<std::string> &GetOutputNames() { return m_reflection.GetOutputNames(); }
	VkPipelineLayout GetPipelineLayout() const { return m_pipelineLayout; }

	void SetRenderPassAndMsaaSamples(VkRenderPass renderPass, VkSampleCountFlagBits msaaSamples) { m_renderPass = renderPass; m_msaaSamples = msaaSamples; }
	void CreateGraphicsPipeline();

	uint32_t AddMesh(const char *name);
	uint32_t AddObject(uint32_t meshIdx);

	uint32_t GetDescriptorPoolsSize(std::vector<uint32_t>& poolsSize) { return _GetDescriptorPoolsSize(poolsSize); }
	void CreateDescriptorSets();
	void UpdateDescriptorSets();
	void DrawObjects(VkCommandBuffer &cb);


	Properties *GetProperties(uint32_t drawObjectId = -1) 
	{
		if (drawObjectId != -1)
		{
			for (auto &p : m_properties)
			{
				if (p.type != Property::PT_EMPTY)
				{
					auto pb = m_shaderProgramParamNames[p.idx];
					auto buf = m_paramsBuffers[pb.dataBufferId].buffer;
					p.val = buf + pb.root->entry.size * drawObjectId + pb.mem->offset;
				}
			}
		}
		return &m_properties; 
	}

	uint32_t GetObjectCount() { return static_cast<uint32_t>(m_drawObjectData.size()); }

	void SetDrawOrder(uint32_t order) { m_drawOrder = order; }
	uint32_t GetDrawOrder() { return m_drawOrder; }

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

	void _BuildProperties();

	void _BuindShaderProgramParamsDesc();
	void _BuindShaderProgramParamsDesc(const std::vector<ValDetails> *vals, uint32_t &dataBufferId);
	void _BuindShaderProgramParamsDesc(const ValMemberDetails & entry, uint32_t parentIdx, const ValDetails & root, uint32_t dataBufferId);
	void _BuildShaderDataBuffers();

	void _CreateNewBufferSet(uint32_t numVertices, uint32_t numIndeces);

	VertexDescription * _GetMeshVertexDescription(const char * name);
	void _ImportMeshData(const VertexDescription *vd, void *dst);
	const VertexAttribs &_GetVertexAttribs() { return m_reflection.GetVertexAttribs(); }

	CShadersReflections          m_reflection;
	OutputWindow                 *vk                     = nullptr;
	VkSampleCountFlagBits        m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkRenderPass                 m_renderPass            = nullptr;

	VkPipelineLayout             m_pipelineLayout        = nullptr;
	VkPipeline                   m_pipeline              = nullptr;

	std::vector<VkDescriptorSetLayout>             m_descriptorSetLayout;

	std::vector<VkVertexInputBindingDescription>   m_bindingDescription;		// <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;		// <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer
//	std::vector<std::vector<uint8_t>>              m_paramsBuffer;
	enum {
		PUSH_CONSTANTS,
		HOSTVISIBLE_UBO,
		UBO
	};
	struct PropertiesBufferInfo
	{
		uint32_t type;
		uint8_t *buffer;
		uint32_t size;
		bool isUpdated;
	};

	std::vector<PropertiesBufferInfo>              m_paramsBuffers;
	std::vector<uint8_t>                           m_paramsLocalBuffer;

	MProperties                        m_properties;
	std::vector<ShaderProgramParamDesc>            m_shaderProgramParamNames;
	bool                                           m_isPushConstantsUsed;

	VkDescriptorSet            m_descriptorSet = nullptr;
	std::vector<UniformBuffer> m_uniformBuffers;

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
	hashtable<const char *, uint32_t> m_meshNames;
	uint32_t m_drawOrder;
};