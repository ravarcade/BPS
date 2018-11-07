struct SwapChainSupportDetails
{
	VkSurfaceCapabilitiesKHR capabilities;
	std::vector<VkSurfaceFormatKHR> formats;
	std::vector<VkPresentModeKHR> presentModes;

	SwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface);
};


struct QueueFamilyIndices
{
	int graphicsFamily = -1;
	int presentFamily = -1;
	int transferFamily = -1;

	bool isComplete()
	{
		return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;
	}

	QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface);
}; 

class OutputWindow
{
	struct UniformBufferObject
	{
//		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

private:
	VkInstance instance;				// required to: (1) create and destroy VkSurfaceKHR, (2) select physical device matching surface
	VkPhysicalDevice physicalDevice;
	int wnd;
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
	VkRenderPass renderPass;
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

	//		CShadersReflections cubeShader;

	// === for demo cube ===
	VkPipeline graphicsPipeline;
	VkBuffer uniformBuffer;
	VkDeviceMemory uniformBufferMemory;
	VkDescriptorPool descriptorPool;
	VkDescriptorSet descriptorSet;
	std::vector<VkCommandBuffer> commandBuffers;

	bool _IsDeviceSuitable(VkPhysicalDevice device);
	QueueFamilyIndices _FindQueueFamilies(VkPhysicalDevice device);
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


	void _CreateDemoCube();
	void _CleanupDemoCube();
	void _CreateUniformBuffer();
	void _CreateDescriptorPool();
	void _CreateDescriptorSet();
	void _CreateCommandBuffers();

	UniformBufferObject *m_ubodata = nullptr;
public:
	OutputWindow();
	void Init();
	void Prepare(VkInstance _instance, int _wnd, GLFWwindow* _window, const VkAllocationCallbacks* _allocator);

	void Cleanup();

	void UpdateUniformBuffer();
	void RecreateSwapChain();
	void DrawFrame();
	bool Exist() { return device != VK_NULL_HANDLE; }
	bool IsValid() {
		if (window)
		{
			window = glfw.GetWnd(wnd);
			return window != nullptr;
		}
		return false;
	}

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

	CShaderProgram cubeShader;
	friend CShaderProgram;
};

class ire {
public:
	struct Vertex {
		glm::vec3 pos;
		DWORD color;

		static VkVertexInputBindingDescription GetBindingDescription() {
			VkVertexInputBindingDescription bindingDescription = {};
			bindingDescription.binding = 0;
			bindingDescription.stride = sizeof(Vertex);
			bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

			return bindingDescription;
		}

		static std::array<VkVertexInputAttributeDescription, 2> GetAttributeDescriptions() {
			std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions = {};

			attributeDescriptions[0].binding = 0;
			attributeDescriptions[0].location = 0;
			attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[0].offset = offsetof(Vertex, pos);

			attributeDescriptions[1].binding = 0;
			attributeDescriptions[1].location = 1;
			attributeDescriptions[1].format = VK_FORMAT_R8G8B8_UNORM;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}

		bool operator== (const Vertex& other) const {
			return pos == other.pos && color == other.color;
		}
	};


	// global:
	VkInstance _instance = VK_NULL_HANDLE;
	const VkAllocationCallbacks* _allocator = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT _callback = VK_NULL_HANDLE;


	// ------------------------------------

	void _ListVKExtensions(std::ostream &out);
	bool _IsValidationLayerSupported();
	std::vector<const char*> _GetRequiredExtensions();
	std::vector<const char*> _GetRequiredLayers();

	VkResult _CreateDebugReportCallbackEXT(VkInstance _instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	void _DestroyDebugReportCallbackEXT(VkInstance _instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);

	// VK intialization steps:
	void CreateInstance();
	void SetupDebugCallback();
	

protected:
	static VKAPI_ATTR VkBool32 VKAPI_CALL _DebugCallback(
		VkDebugReportFlagsEXT flags,
		VkDebugReportObjectTypeEXT objType,
		uint64_t obj,
		size_t location,
		int32_t code,
		const char* layerPrefix,
		const char* msg,
		void* userData);

	static const std::vector<const char*> validationLayers;
	static const std::vector<const char*> deviceExtensions;
	static bool enableValidationLayers;

	static bool _HasStencilComponent(VkFormat format);
	static bool _CheckDeviceExtensionSupport(VkPhysicalDevice device);
	static VkSurfaceFormatKHR _ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	static VkPresentModeKHR _ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	static VkExtent2D _ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow* window);
	static VkSampleCountFlagBits _GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice);
	
public:
	// ============================================================= new code ===

private:

	OutputWindow outputWindows[MAX_WINDOWS];


public:
	ire() :
		_allocator(nullptr),
		_callback(VK_NULL_HANDLE)
	{}

	void Init();
	void Cleanup();
	void Update(float dt);
	void CreateWnd(int wnd, const void *params);
	void Add3DModel(int wnd, const BAMS::RenderingModel *params);

	friend OutputWindow;
};

extern ire re;