
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


class ire {
public:

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

	OutputWindow *outputWindows[MAX_WINDOWS];

public:
	ire() :
		_allocator(nullptr),
		_callback(VK_NULL_HANDLE)
	{}

	void Init();
	void Cleanup();
	void Update(float dt);

	void CreateWnd(const void *params);
	void CloseWnd(const void *params);
	void AddModel(const void *params);
	void AddShader(const void *params);

	void CloseWnd(GLFWwindow* window);
	void AddShader(int wnd, const char *shaderName);
	ModelInfo *AddModel(int wnd, const char *objectName, const BAMS::RENDERINENGINE::VertexDescription *vd, const char *shaderProgram);
	ModelInfo *GetModel(int wnd, const char *objectName);
	uint32_t AddObject(int wnd, ModelInfo * model);


	friend OutputWindow;
};

extern ire re;