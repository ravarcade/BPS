
class OutputWindow; // parent output window class with working interface to Vulkan

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

// descriptor sets managment
struct MiniDescriptorSet
{
	VkDescriptorSet descriptorSet;
	VkDescriptorImageInfo *textures[4];
	uint32_t uniformBufferSet;
	uint32_t hash;
	uint32_t nextEmpty;
	uint32_t refCounter;
	bool rebuildMe;
};

template<>
struct hash<const MiniDescriptorSet *>
{
	U32 operator() (const MiniDescriptorSet *key)
	{
		auto h = JSHash(reinterpret_cast<const U8*>(key->textures), sizeof(key->textures));
		return JSHash(reinterpret_cast<const U8*>(&(key->uniformBufferSet)), sizeof(key->uniformBufferSet), h);
	}
};

template<>
struct cmp<const MiniDescriptorSet *> {
	bool operator()(const MiniDescriptorSet * a, const MiniDescriptorSet * b) 
	{ 
		return a->textures[0] == b->textures[0] &&
			a->textures[1] == b->textures[1] &&
			a->textures[2] == b->textures[2] &&
			a->textures[3] == b->textures[3] &&
			a->uniformBufferSet == b->uniformBufferSet;
	};
};

class CShaderProgram
{
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
		auto *properties = &m_reflection.GetProperties();
		if (drawObjectId != -1)
		{
			for (auto &p : *properties)
			{
				if (p.type != Property::PT_EMPTY)
				{
					auto buf = m_paramsBuffers[p.buffer_idx].buffer;
					p.val = buf + p.buffer_object_stride * drawObjectId + p.buffer_offset;
				}
			}
		}
		return properties;
	}

	uint32_t GetObjectCount() { return static_cast<uint32_t>(m_drawObjectData.size()); }
	void SetDrawOrder(uint32_t order) { m_drawOrder = order; }
	uint32_t GetDrawOrder() { return m_drawOrder; }
	void RebuildAllMiniDescriptorSets(bool forceRebuildMe = false);

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
	void _BuildShaderDataBuffers();

	void _CreateNewBufferSet(uint32_t numVertices, uint32_t numIndeces);

	VertexDescription * _GetMeshVertexDescription(const char * name);
	void _ImportMeshData(const VertexDescription *vd, void *dst);
	const VertexAttribs &_GetVertexAttribs() { return m_reflection.GetVertexAttribs(); }

	CShadersReflections          m_reflection;
	OutputWindow                 *vk = nullptr;
	VkSampleCountFlagBits        m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

	VkRenderPass                 m_renderPass = nullptr;

	VkPipelineLayout             m_pipelineLayout = nullptr;
	VkPipeline                   m_pipeline = nullptr;

	std::vector<VkDescriptorSetLayout>             m_descriptorSetLayout;
	std::vector<VkVertexInputBindingDescription>   m_bindingDescription;		// <- info about: [1] binding point, [2] data stride (in bytes), [3] input rate: per vertex or per instance
	std::vector<VkVertexInputAttributeDescription> m_attributeDescriptions;		// <- info about: [1] location (see vert-shader program), [2] binding (bufer from where data are read), [3] format (flota/int/bool // single val/no. of elements in vector), [4] offset in buffer

	enum {
		PUSH_CONSTANTS,
		HOSTVISIBLE_UBO,
		UBO,
		TEXTURES
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
	uint32_t                                       m_pushConstantsStage;

	uint32_t firstEmptyMiniDescriptorSet = -1;
	std::vector<MiniDescriptorSet> m_miniDescriptorSets;
	hashtable<const MiniDescriptorSet *, uint32_t> m_uniqMiniDescriptoSets;

	void UpdateDescriptorSet(MiniDescriptorSet & mds);

	VkDescriptorSet            m_descriptorSet = nullptr;
	std::vector<UniformBuffer> m_uniformBuffers;

	struct MeshBufferSet {
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
		uint32_t paramsSetId; // used for push constants
		VkDescriptorSet descriptorSet;
		uint32_t meshIdx;
		uint32_t miniDescriptorSetIndex;
		DrawObjectData() :
			paramsSetId(0),
			descriptorSet(0),
			meshIdx(0),
			miniDescriptorSetIndex(-1)
		{}
	};

	std::vector<MeshBufferSet> m_meshBufferSets;
	std::vector<Mesh> m_meshes;
	std::vector<DrawObjectData> m_drawObjectData;
	hashtable<const char *, uint32_t> m_meshNames;
	uint32_t m_drawOrder;
};