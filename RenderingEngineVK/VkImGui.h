

// based on Sascha Willems Vulkan Example - imGui


class VkImGui : public iInputCallback
{
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

	float *pScale;
	float *pTranslate;
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

	VkImGui(OutputWindow *_vk, float width, float height);
	~VkImGui();

	// Initialize styles, keys, etc.
	void init(float width, float height);

	// Initialize all Vulkan resources used by the ui
	void initResources();

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame();

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);

	void _iglfw_cursorPosition(double x, double y);
	void _iglfw_cursorEnter(bool Enter) {};
	void _iglfw_mouseButton(int button, int action, int mods);;
	void _iglfw_scroll(double x, double y);

	void _iglfw_key(int key, int scancode, int action, int mods);
	void _iglfw_character(unsigned int codepoint);

	void _iglfw_resize(int width, int height) { };
	void _iglfw_close() {};

};