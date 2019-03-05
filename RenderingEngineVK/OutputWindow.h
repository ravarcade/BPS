
struct ModelInfo {
	CShaderProgram *shader;
	uint32_t mid;
	ModelInfo() {}
	ModelInfo(CShaderProgram *sh, uint32_t id) : shader(sh), mid(id) {}
};

class OutputWindow
{
private:
	struct SharedUniformBufferObject
	{
		//		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};


	VkInstance instance;				// required to: (1) create and destroy VkSurfaceKHR, (2) select physical device matching surface
	VkPhysicalDevice physicalDevice;
	GLFWwindow* window;
	const VkAllocationCallbacks* allocator;

	// ---- 
	VkSurfaceKHR surface;
	VkDevice device; // logical device
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;
	VkRenderPass renderPass = nullptr;
	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;
	VkImage colorImage;
	VkDeviceMemory colorImageMemory;
	VkImageView colorImageView;
	VkCommandPool commandPool;
	VkCommandPool transferPool;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkBuffer        sharedUniformBuffer = nullptr;
	VkDeviceMemory  sharedUniformBufferMemory = nullptr;
	//		CShadersReflections cubeShader;
	VkViewport viewport;
	VkRect2D scissor;
	// === for demo cube ===
	VkDescriptorPool descriptorPool;
	VkDescriptorSet currentDescriptorSet;
	std::vector<VkCommandBuffer> commandBuffers;
	bool recreateCommandBuffers = false;

	bool _IsDeviceSuitable(VkPhysicalDevice device);
	VkFormat _FindDepthFormat();
	VkFormat _FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// image functions
	void _CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);
	VkImageView _CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void _TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	uint32_t _FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkCommandBuffer _BeginSingleTimeCommands();
	void _EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void _CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	void _CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void * srcData, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
	void _CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
	VkShaderModule _CreateShaderModule(const char * shaderName);

	bool _CreateSwapChain();
	void _CleanupSwapChain();

	void _CreateSharedUniform();
	void _CleanupSharedUniform();

	void _CreateRenderPass();
	void _CleanupRenderPass();

	void _CreateDemoCube();
	void _CreateDescriptorPool();
	void _CreateCommandBuffers();
	void _RecreateCommandBuffers();
	void _CreateSimpleRenderPass(VkFormat format, VkSampleCountFlagBits samples);

	SharedUniformBufferObject *sharedUboData = nullptr;

	void _LoadShaderPrograms();
	void _CleanupShaderPrograms();
	void _Cleanup();

public:
	OutputWindow();
	~OutputWindow() { _Cleanup(); }
	void Init();
	void Prepare(VkInstance _instance, GLFWwindow* _window, const VkAllocationCallbacks* _allocator);
	void Close(GLFWwindow *wnd = nullptr);

	void UpdateUniformBuffer();
	void RecreateSwapChain();
	void DrawFrame();
	bool Exist() { return device != VK_NULL_HANDLE; }
	void BufferRecreationNeeded() { recreateCommandBuffers = true; }
	bool IsValid() { return window != nullptr; }

	template<typename T> void vkDestroy(T &v) { if (v) { vkDestroyDevice(v, allocator); v = VK_NULL_HANDLE; } }

#ifdef Define_vkDestroy
#undef Define_vkDestroy
#endif
#define Define_vkDestroy(t) template<> void vkDestroy(Vk##t &v) { if (v) { vkDestroy##t(device, v, allocator); v = VK_NULL_HANDLE; } }
	Define_vkDestroy(Pipeline);
	Define_vkDestroy(PipelineLayout);
	Define_vkDestroy(ImageView);
	Define_vkDestroy(Image);
	Define_vkDestroy(Framebuffer);
	Define_vkDestroy(RenderPass);
	Define_vkDestroy(SwapchainKHR);
	Define_vkDestroy(DescriptorPool);
	Define_vkDestroy(DescriptorSetLayout);
	Define_vkDestroy(Buffer);
	Define_vkDestroy(Semaphore);
	Define_vkDestroy(CommandPool);
	Define_vkDestroy(ShaderModule);
	template<> void vkDestroy(VkSurfaceKHR &v) { if (v) { vkDestroySurfaceKHR(instance, v, allocator); v = VK_NULL_HANDLE; } }

	void vkFree(VkDeviceMemory &v) { if (v) { vkFreeMemory(device, v, allocator); v = nullptr; } }
	void vkFree(VkCommandPool &v1, std::vector<VkCommandBuffer> &v2) { if (v1 && v2.size()) { vkFreeCommandBuffers(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }
	void vkFree(VkDescriptorPool &v1, std::vector<VkDescriptorSet> &v2) { if (v1 && v2.size()) { vkFreeDescriptorSets(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }

	BAMS::CORE::CStringHastable<ModelInfo> models;
	BAMS::CORE::CStringHastable<CShaderProgram> shaders;

	void AddShader(const char * name);
	ModelInfo *AddModel(const char *name, const BAMS::RENDERINENGINE::VertexDescription *vd, const char *shaderProgram);
	ModelInfo *GetModel(const char *name);
	//	CShaderProgram cubeShader;
	friend CShaderProgram;
};
