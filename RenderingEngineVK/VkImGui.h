

// based on Sascha Willems Vulkan Example - imGui


class VkImGui {
private:
	// Vulkan resources for rendering the UI
	VkBuffer vertexBuf;
	VkDeviceMemory vertexMem;
	VkBuffer indexBuf;
	VkDeviceMemory indexMem;
	void *vertexData;
	void *indexData;

//	vks::Buffer vertexBuffer;
//	vks::Buffer indexBuffer;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;

	CTexture2d *fontTexture; // all below things are inside fontTexture object
	//VkSampler sampler;
	//VkDeviceMemory fontMemory = VK_NULL_HANDLE;
	//VkImage fontImage = VK_NULL_HANDLE;
	//VkImageView fontView = VK_NULL_HANDLE;


	VkPipelineCache pipelineCache;
	VkPipelineLayout pipelineLayout;
	VkPipeline pipeline;
	VkDescriptorPool descriptorPool;
	VkDescriptorSetLayout descriptorSetLayout;
	VkDescriptorSet descriptorSet;
//	VkDevice *logicalDevice;
	OutputWindow *vk;
public:
	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	VkImGui(OutputWindow *_vk);
	~VkImGui();

	// Initialize styles, keys, etc.
	void init(float width, float height);

	// Initialize all Vulkan resources used by the ui
	void initResources(VkRenderPass renderPass, VkQueue copyQueue);

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame(bool updateFrameGraph);

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);

};