// RenderingEngineVK.cpp : Defines the exported functions for the DLL application.
//

#include "stdafx.h"
#include "BAMEngine.h"

using namespace BAMS;

#pragma pack(push,1)
struct Point3D { float x, y, z; };
struct Point2D { float x, y; };

struct Model 
{
	uint32_t noVertices;
	uint32_t noFaces;
	Point3D *vertices;
	Point3D *normals;
	Point2D *textCoord;
	uint32_t *faces;
};

#pragma pack(pop)

Point3D defaultBoxVertices[] = {
	{ -1, -1, -1 },
	{  1, -1, -1 },
	{  1,  1, -1 },
	{ -1,  1, -1 },

	{ -1, -1,  1 },
	{  1, -1,  1 },
	{  1,  1,  1 },
	{ -1,  1,  1 }
};

uint32_t defaultBoxFaces[] = {
	0, 1, 2, 2, 3, 0,
	0, 4, 5, 1, 0, 5,
};

Model defaultBox = {
	8, 16,
	defaultBoxVertices,
	nullptr,
	nullptr,
	defaultBoxFaces
};

// ============================================================================ iglwf ====

class iglwf {
	static void ErrorCallback(int err, const char* err_str)
	{
		GLFW_LastErr = err;
		GLFW_LastErrStr = err_str;
	}


public:
	static int GLFW_LastErr;
	static const char *GLFW_LastErrStr;
	GLFWwindow* main3Dwindow;
	int main3DWindow_width;
	int main3DWindow_height;


	iglwf() :
		main3Dwindow(nullptr),
		main3DWindow_width(0),
		main3DWindow_height(0)
	{}

	void Init()
	{
		glfwSetErrorCallback(ErrorCallback);
		glfwInit();
	}

	void Cleanup()
	{
		if (main3Dwindow)
			glfwDestroyWindow(main3Dwindow);
		glfwTerminate();
	}

	void Update(float dt)
	{
		if (main3Dwindow) {
			if (!glfwWindowShouldClose(main3Dwindow))
			{
				glfwPollEvents();
			}
			else
			{
				glfwDestroyWindow(main3Dwindow);
				main3Dwindow = nullptr;
			}
		}
	}

	// --------------------------------------------------

	void Create_main3dwindow(int width, int height)
	{
		main3DWindow_width = width;
		main3DWindow_height = height;

		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
		main3Dwindow = glfwCreateWindow(main3DWindow_width, main3DWindow_height, "Rendering Engine: Vulkan", nullptr, nullptr);
	}
};

int iglwf::GLFW_LastErr = 0;
const char *iglwf::GLFW_LastErrStr = nullptr;
iglwf glwf;

// ============================================================================

class ivk {
	struct QueueFamilyIndices 
	{
		int graphicsFamily = -1;
		int presentFamily = -1;
		int transferFamily = -1;

		bool isComplete() 
		{
			return graphicsFamily >= 0 && presentFamily >= 0 && transferFamily >= 0;
		}
	};

	struct SwapChainSupportDetails 
	{
		VkSurfaceCapabilitiesKHR capabilities;
		std::vector<VkSurfaceFormatKHR> formats;
		std::vector<VkPresentModeKHR> presentModes;
	};
public:
	struct Vertex {
		glm::vec3 pos;
		glm::vec3 color;

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
			attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
			attributeDescriptions[1].offset = offsetof(Vertex, color);

			return attributeDescriptions;
		}

		bool operator== (const Vertex& other) const {
			return pos == other.pos && color == other.color;
		}
	};

	struct UniformBufferObject {
		glm::mat4 model;
		glm::mat4 view;
		glm::mat4 proj;
	};

	// ---------------------------------------
	VkInstance _instance = VK_NULL_HANDLE;
	VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
	VkSurfaceKHR _surface = VK_NULL_HANDLE;
	VkDevice _device = VK_NULL_HANDLE;
	VkQueue _graphicsQueue = VK_NULL_HANDLE;
	VkQueue _presentQueue = VK_NULL_HANDLE;
	VkQueue _transferQueue = VK_NULL_HANDLE;
	VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
	std::vector<VkImage> _swapChainImages;
	std::vector<VkImageView> _swapChainImageViews;
	std::vector<VkFramebuffer> _swapChainFramebuffers;
	VkFormat _swapChainImageFormat;
	VkExtent2D _swapChainExtent;
	VkRenderPass _renderPass = VK_NULL_HANDLE;
	VkDescriptorSetLayout _descriptorSetLayout;
	VkPipelineLayout _pipelineLayout;
	VkPipeline _graphicsPipeline;
	VkCommandPool _commandPool = VK_NULL_HANDLE;
	VkCommandPool _transferPool = VK_NULL_HANDLE;

	VkBuffer _vertexBuffer;
	VkDeviceMemory _vertexBufferMemory;

	VkBuffer _indexBuffer;
	VkDeviceMemory _indexBufferMemory;

	VkBuffer _uniformBuffer;
	VkDeviceMemory _uniformBufferMemory;

	VkDescriptorPool _descriptorPool;
	VkDescriptorSet _descriptorSet;

	VkImage _depthImage;
	VkDeviceMemory _depthImageMemory;
	VkImageView _depthImageView;

	std::vector<VkCommandBuffer> _commandBuffers;
	VkSemaphore _imageAvailableSemaphore;
	VkSemaphore _renderFinishedSemaphore;

	const VkAllocationCallbacks* _allocator = VK_NULL_HANDLE;
	VkDebugReportCallbackEXT _callback = VK_NULL_HANDLE;

	bool enableValidationLayers = false;

	// ------------------------------------

	void ListVKExtensions(std::ostream &out);
	bool IsValidationLayerSupported();
	std::vector<const char*> GetRequiredExtensions();
	std::vector<const char*> GetRequiredLayers();
	bool IsDeviceSuitable(VkPhysicalDevice device);
	VkResult CreateDebugReportCallbackEXT(VkInstance _instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback);
	void DestroyDebugReportCallbackEXT(VkInstance _instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator);
	QueueFamilyIndices FindQueueFamilies(VkPhysicalDevice device);
	bool CheckDeviceExtensionSupport(VkPhysicalDevice device);
	SwapChainSupportDetails QuerySwapChainSupport(VkPhysicalDevice device);

	VkSurfaceFormatKHR ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
	VkPresentModeKHR ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes);
	VkExtent2D ChooseSwapExtent(const VkSurfaceCapabilitiesKHR & capabilities, GLFWwindow* window);
	VkImageView CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);

	VkFormat FindDepthFormat();
	bool HasStencilComponent(VkFormat format);
	VkFormat FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// VK intialization steps:
	void CreateInstance();
	void SetupDebugCallback();

	void CreateSurface(GLFWwindow* window, VkSurfaceKHR* surface);
	void SelectPhysicalDevice();
	void CreateLogicalDevice();
	void CreateSwapChain(GLFWwindow* window);
	void CreateImageViews();
	void CreateRenderPass();
	void CreateDescriptorSetLayout();
	void CreateGraphicsPipeline();
	void CreateCommandPool();
	void CreateDepthResources();

	void CreateVertexBuffer();

	void CreateIndexBuffer();

	void CreateUniformBuffer();

	void CreateDescriptorPool();

	void CreateDescriptorSet();

	void CreateCommandBuffers();

	void CreateSemaphores();

	VkCommandBuffer BeginSingleTimeCommands();
	void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	void CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);

	void CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void * srcData, VkBuffer & buffer, VkDeviceMemory & bufferMemory);

	void CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	void CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);

	uint32_t FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	void CreateFramebuffers();

	VkShaderModule CreateShaderModule(const char * shaderName);

	static VKAPI_ATTR VkBool32 VKAPI_CALL DebugCallback(
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
public:
	ivk() : 
		enableValidationLayers(false),
		_allocator(nullptr),
		_callback(VK_NULL_HANDLE)
	{}

	void Init();

	void Cleanup();
	void CleanupSwapChain();

	void UpdateUniformBuffer();

	void RecreateSwapChain();

	void DrawFrame();

	void Update(float dt);

	void Create_main3dwindow();
};
const std::vector<const char*> ivk::validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<const char*> ivk::deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };

const std::vector<ivk::Vertex> vertices = {
	{ { -0.5f, -0.5f,  0.5f },{ 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, -0.5f,  0.5f },{ 0.0f, 1.0f, 0.0f } },
	{ { 0.5f,  0.5f,  0.5f },{ 0.0f, 0.0f, 1.0f } },
	{ { -0.5f,  0.5f,  0.5f },{ 1.0f, 1.0f, 1.0f } },
	{ { -0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f, 0.0f } },
	{ { 0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f, 0.0f } },
	{ { 0.5f,  0.5f, -0.5f },{ 0.0f, 0.0f, 1.0f } },
	{ { -0.5f,  0.5f, -0.5f },{ 1.0f, 1.0f, 1.0f } }
};

const std::vector<uint16_t> indices = {
	0, 1, 2,  2, 3, 0,
	0, 4, 5,  1, 0, 5,
	1, 5, 6,  2, 1, 6,
	2, 6, 7,  3, 2, 7,
	3, 7, 4,  0, 3, 4,
	5, 4, 6,  7, 6, 4
};

ivk vk;

void ivk::Init()
{
	CreateInstance();
	SetupDebugCallback();
}

void ivk::Cleanup()
{
	if (_instance)
	{
		CleanupSwapChain();
		if (_transferPool != VK_NULL_HANDLE)
			vkDestroyCommandPool(_device, _transferPool, _allocator);
		vkDestroyCommandPool(_device, _commandPool, _allocator);

		vkDestroyDescriptorPool(_device, _descriptorPool, _allocator);
		vkDestroyDescriptorSetLayout(_device, _descriptorSetLayout, _allocator);

		vkDestroySemaphore(_device, _renderFinishedSemaphore, _allocator);
		vkDestroySemaphore(_device, _imageAvailableSemaphore, _allocator);

		vkDestroyBuffer(_device, _uniformBuffer, _allocator);
		vkFreeMemory(_device, _uniformBufferMemory, _allocator);


		vkDestroyBuffer(_device, _indexBuffer, _allocator);
		vkFreeMemory(_device, _indexBufferMemory, _allocator);

		vkDestroyBuffer(_device, _vertexBuffer, _allocator);
		vkFreeMemory(_device, _vertexBufferMemory, _allocator);
		
		vkDestroyDevice(_device, _allocator);
		vkDestroySurfaceKHR(_instance, _surface, _allocator);
		DestroyDebugReportCallbackEXT(_instance, _callback, _allocator);
		vkDestroyInstance(_instance, _allocator);

		_commandPool = VK_NULL_HANDLE;
		_transferPool = VK_NULL_HANDLE;
		_swapChain = VK_NULL_HANDLE;
		_device = VK_NULL_HANDLE;
		_surface = VK_NULL_HANDLE;
		_instance = VK_NULL_HANDLE;
	}
}

void ivk::CleanupSwapChain()
{
	vkDestroyImageView(_device, _depthImageView, _allocator);
	vkDestroyImage(_device, _depthImage, _allocator);
	vkFreeMemory(_device, _depthImageMemory, _allocator);

	for (size_t i = 0; i < _swapChainFramebuffers.size(); i++) {
		vkDestroyFramebuffer(_device, _swapChainFramebuffers[i], _allocator);
	}

	vkFreeCommandBuffers(_device, _commandPool, static_cast<uint32_t>(_commandBuffers.size()), _commandBuffers.data());

	vkDestroyPipeline(_device, _graphicsPipeline, _allocator);
	vkDestroyPipelineLayout(_device, _pipelineLayout, _allocator);
	vkDestroyRenderPass(_device, _renderPass, _allocator);

	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		vkDestroyImageView(_device, _swapChainImageViews[i], _allocator);
	}

	vkDestroySwapchainKHR(_device, _swapChain, _allocator);
}

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

void ivk::UpdateUniformBuffer() 
{
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;

	UniformBufferObject ubo = {};
	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
//	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	ubo.proj = glm::perspective(glm::radians(45.0f), _swapChainExtent.width / (float)_swapChainExtent.height, 0.1f, 10.0f);
	ubo.proj[1][1] *= -1;

	void* data;
	vkMapMemory(_device, _uniformBufferMemory, 0, sizeof(ubo), 0, &data);
	memcpy(data, &ubo, sizeof(ubo));
	vkUnmapMemory(_device, _uniformBufferMemory);
}

void ivk::RecreateSwapChain() 
{
	vkDeviceWaitIdle(_device);

	CleanupSwapChain();

	CreateSwapChain(glwf.main3Dwindow);
	CreateImageViews();
	CreateRenderPass();
	CreateGraphicsPipeline();
	CreateDepthResources();
	CreateFramebuffers();
	CreateCommandBuffers();
}

void ivk::DrawFrame() 
{
	uint32_t imageIndex;
	VkResult result = vkAcquireNextImageKHR(_device, _swapChain, std::numeric_limits<uint64_t>::max(), _imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR) 
	{
		RecreateSwapChain();
		return;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
		throw std::runtime_error("failed to acquire swap chain image!");
	}

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { _imageAvailableSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &_commandBuffers[imageIndex];

	VkSemaphore signalSemaphores[] = { _renderFinishedSemaphore };
	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	if (vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = signalSemaphores;

	VkSwapchainKHR swapChains[] = { _swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; // Optional

	result = vkQueuePresentKHR(_presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}

	vkQueueWaitIdle(_presentQueue);
}

void ivk::Update(float dt)
{
	UpdateUniformBuffer();
	DrawFrame();
}

void ivk::Create_main3dwindow()
{
	CreateSurface(glwf.main3Dwindow, &_surface);
	SelectPhysicalDevice();
	CreateLogicalDevice();
	CreateSwapChain(glwf.main3Dwindow);
	CreateImageViews();
	CreateRenderPass();
	CreateDescriptorSetLayout();
	CreateGraphicsPipeline();
	CreateCommandPool();
	CreateDepthResources();
	CreateFramebuffers();

	CreateVertexBuffer();
	CreateIndexBuffer();
	CreateUniformBuffer();
	CreateDescriptorPool();
	CreateDescriptorSet();
	CreateCommandBuffers();
	CreateSemaphores();
}

/// <summary>
/// Creates vulkan instance.
/// [Init. Step 1.]
/// </summary>
void ivk::CreateInstance()
{
#ifdef _DEBUG
	enableValidationLayers = true;
#endif

	ListVKExtensions(std::cout);
	if (enableValidationLayers && !IsValidationLayerSupported())
	{
		throw std::runtime_error("validation layers requested, but not available!");
	}

	// TODO: Fill ApplicationName, EngineName with real info. Now it is only "placeholder"
	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "BAMS";
	appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.pEngineName = "BAMS Rendering Engine";
	appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	appInfo.apiVersion = VK_API_VERSION_1_0;

	auto extensions = GetRequiredExtensions();
	auto layers = GetRequiredLayers();

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
	createInfo.ppEnabledExtensionNames = extensions.size() ? extensions.data() : nullptr;
	createInfo.enabledLayerCount = static_cast<uint32_t>(layers.size());
	createInfo.ppEnabledLayerNames = layers.size() ? layers.data() : nullptr;

	if (vkCreateInstance(&createInfo, _allocator, &_instance) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create instance!");
	}
}

/// <summary>
/// Setups the debug callback.
/// [Init. Step 2.]
/// </summary>
void ivk::SetupDebugCallback()
{
	if (enableValidationLayers)
	{

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = DebugCallback;

		if (CreateDebugReportCallbackEXT(_instance, &createInfo, _allocator, &_callback) != VK_SUCCESS) 
		{
			throw std::runtime_error("failed to set up debug callback!");
		}
	}
}

/// <summary>
/// Creates the surface.
/// [Init. Step 3.]
/// </summary>
/// <param name="window">The window.</param>
/// <param name="surface">The surface.</param>
void ivk::CreateSurface(GLFWwindow* window, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(_instance, window, _allocator, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

/// <summary>
/// Selects the physical device.
/// [Init. Step 4.]
/// </summary>
void ivk::SelectPhysicalDevice()
{
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(_instance, &deviceCount, nullptr);
	if (deviceCount == 0) 
	{
		throw std::runtime_error("failed to find GPUs with Vulkan support!");
	}

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(_instance, &deviceCount, devices.data());

	for (const auto& device : devices) 
	{
		if (IsDeviceSuitable(device)) 
		{
			_physicalDevice = device;
			break;
		}
	}

	if (_physicalDevice == VK_NULL_HANDLE) 
	{
		throw std::runtime_error("failed to find a suitable GPU!");
	}
}

/// <summary>
/// Vulkan debug callback function.
/// [Init. Step 2.]
/// See: https://vulkan.lunarg.com/doc/view/1.0.37.0/linux/vkspec.chunked/ch32s02.html
/// </summary>
/// <param name="flags">The flags.</param>
/// <param name="objType">Type of the object.</param>
/// <param name="obj">The object.</param>
/// <param name="location">The location.</param>
/// <param name="code">The code.</param>
/// <param name="layerPrefix">The layer prefix.</param>
/// <param name="msg">The message.</param>
/// <param name="userData">The user data.</param>
/// <returns></returns>
VKAPI_ATTR VkBool32 VKAPI_CALL ivk::DebugCallback(
	VkDebugReportFlagsEXT flags,
	VkDebugReportObjectTypeEXT objType,
	uint64_t obj,
	size_t location,
	int32_t code,
	const char* layerPrefix,
	const char* msg,
	void* userData)
{

	std::cerr << "validation layer: " << msg << std::endl;
	//errorInConsoleWindow = true;
	return VK_FALSE;
}

/// <summary>
/// Checks if physical device is suitable for us.
/// [Init. Step 4.1.]
/// </summary>
/// <param name="device">The device.</param>
/// <returns>
///   <c>true</c> if physical device is suitable; otherwise, <c>false</c>.
/// </returns>
bool ivk::IsDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices = FindQueueFamilies(device);

	bool extensionsSupported = CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported) 
	{
		SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(device);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		//			deviceFeatures.geometryShader &&
		indices.isComplete() &&
		extensionsSupported &&
		deviceFeatures.samplerAnisotropy &&
		swapChainAdequate;
}

void ivk::CreateLogicalDevice() 
{
	// queues: one from graphics family one from present family (it may be same)
	QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily, indices.transferFamily };

	float queuePriority = 1.0f;
	for (int queueFamily : uniqueQueueFamilies) {
		VkDeviceQueueCreateInfo queueCreateInfo = {};
		queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfo.queueFamilyIndex = queueFamily;
		queueCreateInfo.queueCount = 1;
		queueCreateInfo.pQueuePriorities = &queuePriority;
		queueCreateInfos.push_back(queueCreateInfo);
	}


	// features
	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	// createInfo structure to create logical device
	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
	createInfo.pQueueCreateInfos = queueCreateInfos.data();
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = deviceExtensions.data();

	// add validation layer for logical device
	if (enableValidationLayers) 
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
		createInfo.ppEnabledLayerNames = validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(_physicalDevice, &createInfo, _allocator, &_device) != VK_SUCCESS) 
	{
		throw std::runtime_error("failed to create logical device!");
	}

	vkGetDeviceQueue(_device, indices.graphicsFamily, 0, &_graphicsQueue);
	vkGetDeviceQueue(_device, indices.presentFamily, 0, &_presentQueue);
	vkGetDeviceQueue(_device, indices.transferFamily, 0, &_transferQueue);
}

void ivk::CreateSwapChain(GLFWwindow* window)
{
	SwapChainSupportDetails swapChainSupport = QuerySwapChainSupport(_physicalDevice);

	VkSurfaceFormatKHR surfaceFormat = ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ChooseSwapPresentMode(swapChainSupport.presentModes);
	VkExtent2D extent = ChooseSwapExtent(swapChainSupport.capabilities, window);

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount) 
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = _surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // VK_IMAGE_USAGE_TRANSFER_DST_BIT 

	QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);
	uint32_t queueFamilyIndices[] = { (uint32_t)indices.graphicsFamily, (uint32_t)indices.presentFamily };

	if (indices.graphicsFamily != indices.presentFamily) {
		createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		createInfo.queueFamilyIndexCount = 2;
		createInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else {
		createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		createInfo.queueFamilyIndexCount = 0; // Optional
		createInfo.pQueueFamilyIndices = nullptr; // Optional
	}

	createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
	createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;

	createInfo.presentMode = presentMode;
	createInfo.clipped = VK_TRUE;

	createInfo.oldSwapchain = VK_NULL_HANDLE;

	if (vkCreateSwapchainKHR(_device, &createInfo, _allocator, &_swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, nullptr);
	_swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(_device, _swapChain, &imageCount, _swapChainImages.data());

	_swapChainImageFormat = surfaceFormat.format;
	_swapChainExtent = extent;
}

void ivk::CreateImageViews() {
	_swapChainImageViews.resize(_swapChainImages.size());

	for (size_t i = 0; i < _swapChainImages.size(); i++) {
		_swapChainImageViews[i] = CreateImageView(_swapChainImages[i], _swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}
}

void ivk::CreateGraphicsPipeline()
{
	// compile programs
	VkShaderModule vertShaderModule = CreateShaderModule("vert");
	VkShaderModule fragShaderModule = CreateShaderModule("frag");

	// put fragment and vertex shader in one program 
	VkPipelineShaderStageCreateInfo vertShaderStageInfo = {};
	vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertShaderStageInfo.module = vertShaderModule;
	vertShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragShaderStageInfo = {};
	fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragShaderStageInfo.module = fragShaderModule;
	fragShaderStageInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = { vertShaderStageInfo, fragShaderStageInfo };

	// from vertex shader get binding descriptions and attributes
	std::vector<VkVertexInputBindingDescription> bindingDescription(0);
	std::vector<VkVertexInputAttributeDescription> attributeDescriptions;

	bindingDescription.push_back(Vertex::GetBindingDescription());
	auto att = Vertex::GetAttributeDescriptions();
	attributeDescriptions = std::vector<VkVertexInputAttributeDescription>(att.begin(), att.end());


	// set binging desc and attributes desc
	VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
	vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

	vertexInputInfo.vertexBindingDescriptionCount = static_cast<uint32_t>(bindingDescription.size());
	vertexInputInfo.pVertexBindingDescriptions = bindingDescription.data();

	vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
	vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();


	// how we will draw: triangles
	VkPipelineInputAssemblyStateCreateInfo inputAssembly = {};
	inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssembly.primitiveRestartEnable = VK_FALSE;

	// view port: whole window
	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float)_swapChainExtent.width;
	viewport.height = (float)_swapChainExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	// no scisors
	VkRect2D scissor = {};
	scissor.offset = { 0, 0 };
	scissor.extent = _swapChainExtent;

	// viewport and scisors in one structure: viewportState
	VkPipelineViewportStateCreateInfo viewportState = {};
	viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportState.viewportCount = 1;
	viewportState.pViewports = &viewport;
	viewportState.scissorCount = 1;
	viewportState.pScissors = &scissor;

	VkPipelineRasterizationStateCreateInfo rasterizer = {};
	rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizer.depthClampEnable = VK_FALSE;
	rasterizer.rasterizerDiscardEnable = VK_FALSE;
	rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizer.lineWidth = 1.0f;
	rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
	rasterizer.depthBiasEnable = VK_FALSE;
	rasterizer.depthBiasConstantFactor = 0.0f; // Optional
	rasterizer.depthBiasClamp = 0.0f; // Optional
	rasterizer.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisampling = {};
	multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisampling.sampleShadingEnable = VK_FALSE;
	multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisampling.minSampleShading = 1.0f; // Optional
	multisampling.pSampleMask = nullptr; // Optional
	multisampling.alphaToCoverageEnable = VK_FALSE; // Optional
	multisampling.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineDepthStencilStateCreateInfo depthStencil = {};
	depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencil.depthTestEnable = VK_TRUE;
	depthStencil.depthWriteEnable = VK_TRUE;
	depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencil.depthBoundsTestEnable = VK_FALSE;
	depthStencil.minDepthBounds = 0.0f; // Optional
	depthStencil.maxDepthBounds = 1.0f; // Optional
	depthStencil.stencilTestEnable = VK_FALSE;
	depthStencil.front = {}; // Optional
	depthStencil.back = {}; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachment = {};
	colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	colorBlendAttachment.blendEnable = VK_FALSE;
	colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD; // Optional

														 /* alpha blending
														 colorBlendAttachment.blendEnable = VK_TRUE;
														 colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
														 colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
														 colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
														 colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
														 colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
														 colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
														 */

	VkPipelineColorBlendStateCreateInfo colorBlending = {};
	colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlending.logicOpEnable = VK_FALSE;
	colorBlending.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlending.attachmentCount = 1;
	colorBlending.pAttachments = &colorBlendAttachment;
	colorBlending.blendConstants[0] = 0.0f; // Optional
	colorBlending.blendConstants[1] = 0.0f; // Optional
	colorBlending.blendConstants[2] = 0.0f; // Optional
	colorBlending.blendConstants[3] = 0.0f; // Optional

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_LINE_WIDTH
	};

	VkPipelineDynamicStateCreateInfo dynamicState = {};
	dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicState.dynamicStateCount = 2;
	dynamicState.pDynamicStates = dynamicStates;

	VkPipelineLayoutCreateInfo pipelineLayoutInfo = {};
	pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutInfo.setLayoutCount = 1;
	pipelineLayoutInfo.pSetLayouts = &_descriptorSetLayout;
	pipelineLayoutInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutInfo.pPushConstantRanges = 0; // Optional

	if (vkCreatePipelineLayout(_device, &pipelineLayoutInfo, _allocator, &_pipelineLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create pipeline layout!");
	}


	VkGraphicsPipelineCreateInfo pipelineInfo = {};
	pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	pipelineInfo.stageCount = 2;
	pipelineInfo.pStages = shaderStages;
	pipelineInfo.pVertexInputState = &vertexInputInfo;
	pipelineInfo.pInputAssemblyState = &inputAssembly;
	pipelineInfo.pViewportState = &viewportState;
	pipelineInfo.pRasterizationState = &rasterizer;
	pipelineInfo.pMultisampleState = &multisampling;
	pipelineInfo.pDepthStencilState = &depthStencil;
	pipelineInfo.pColorBlendState = &colorBlending;
	pipelineInfo.pDynamicState = nullptr; // Optional
	pipelineInfo.layout = _pipelineLayout;
	pipelineInfo.renderPass = _renderPass;
	pipelineInfo.subpass = 0;
	pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	pipelineInfo.basePipelineIndex = -1; // Optional
	if (vkCreateGraphicsPipelines(_device, VK_NULL_HANDLE, 1, &pipelineInfo, _allocator, &_graphicsPipeline) != VK_SUCCESS) {
		throw std::runtime_error("failed to create graphics pipeline!");
	}
	vkDestroyShaderModule(_device, fragShaderModule, _allocator);
	vkDestroyShaderModule(_device, vertShaderModule, _allocator);
}

void ivk::CreateCommandPool() 
{
	QueueFamilyIndices queueFamilyIndices = FindQueueFamilies(_physicalDevice);

	VkCommandPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
	poolInfo.flags = 0; // Optional

	if (vkCreateCommandPool(_device, &poolInfo, _allocator, &_commandPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create command pool!");
	}

	if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.transferFamily)
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
		poolInfo.flags = 0; // Optional

		if (vkCreateCommandPool(_device, &poolInfo, _allocator, &_transferPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}

	}
}

void ivk::CreateFramebuffers()
{
	_swapChainFramebuffers.resize(_swapChainImageViews.size());

	for (size_t i = 0; i < _swapChainImageViews.size(); i++) {
		std::array<VkImageView, 2> attachments = {
			_swapChainImageViews[i],
			_depthImageView
		};


		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = _renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = _swapChainExtent.width;
		framebufferInfo.height = _swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(_device, &framebufferInfo, _allocator, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to create framebuffer!");
		}
	}
}

VkShaderModule ivk::CreateShaderModule(const char *shaderName) 
{
	BAMS::CResourceManager rm;
	auto code = rm.GetRawDataByName(shaderName);
	if (!code.IsLoaded())
		rm.LoadSync();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.GetSize();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.GetData());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(_device, &createInfo, _allocator, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}

void ivk::CreateRenderPass() 
{
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = _swapChainImageFormat;
	colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = FindDepthFormat();
	depthAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;


	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstSubpass = 0;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;


	std::array<VkAttachmentDescription, 2> attachments = { colorAttachment, depthAttachment };

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(_device, &renderPassInfo, _allocator, &_renderPass) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create render pass!");
	}

}

void ivk::CreateDescriptorSetLayout()
{
	VkDescriptorSetLayoutBinding uboLayoutBinding = {};
	uboLayoutBinding.binding = 0;
	uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	uboLayoutBinding.descriptorCount = 1;
	uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	uboLayoutBinding.pImmutableSamplers = nullptr; // Optional

	/*
	VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
	samplerLayoutBinding.binding = 1;
	samplerLayoutBinding.descriptorCount = 1;
	samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	samplerLayoutBinding.pImmutableSamplers = nullptr;
	samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

	std::array<VkDescriptorSetLayoutBinding, 2> bindings = { uboLayoutBinding, samplerLayoutBinding };
	*/
	std::array<VkDescriptorSetLayoutBinding, 1> bindings = { uboLayoutBinding };
	VkDescriptorSetLayoutCreateInfo layoutInfo = {};
	layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
	layoutInfo.pBindings = bindings.data();


	if (vkCreateDescriptorSetLayout(_device, &layoutInfo, _allocator, &_descriptorSetLayout) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor set layout!");
	}
}

/// <summary>
/// Checks the vulkan validation layer support.
/// [Init. Step 1.1.]
/// </summary>
/// <returns></returns>
bool ivk::IsValidationLayerSupported()
{
	uint32_t layerCount;
	vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

	std::vector<VkLayerProperties> availableLayers(layerCount);
	vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

	for (const char* layerName : validationLayers)
	{
		bool layerFound = false;

		for (const auto& layerProperties : availableLayers) 
		{
			if (strcmp(layerName, layerProperties.layerName) == 0) 
			{
				layerFound = true;
				break;
			}
		}

		if (!layerFound) 
		{
			return false;
		}
	}

	return true;
}

/// <summary>
/// Gets the required vulkan extensions.
/// </summary>
/// <returns></returns>
std::vector<const char*> ivk::GetRequiredExtensions()
{
	std::vector<const char*> extensions;

	uint32_t glfwExtensionCount = 0;
	const char** glfwExtensions;
	glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	for (unsigned int i = 0; i < glfwExtensionCount; i++)
	{
		extensions.push_back(glfwExtensions[i]);
	}

	if (enableValidationLayers)
	{
		extensions.push_back(VK_EXT_DEBUG_REPORT_EXTENSION_NAME);
	}

	return extensions;
}

/// <summary>
/// Gets the required vulkan layers (validation layer if debug is enabled).
/// </summary>
/// <returns></returns>
std::vector<const char*> ivk::GetRequiredLayers()
{
	return enableValidationLayers ? validationLayers : std::vector<const char *>();
}

/// <summary>
/// Lists the vulkan extensions.
/// Debug only.
/// </summary>
/// <param name="out">The out. ostream.</param>
void ivk::ListVKExtensions(std::ostream &out)
{
	uint32_t extensionCount = 0;
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	VkExtensionProperties *extensions = new VkExtensionProperties[extensionCount];
	vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions);

	out << "available extensions:" << std::endl;
	for (uint32_t i = 0; i < extensionCount; ++i)
	{
		out << "\t" << extensions[i].extensionName << std::endl;
	}

	delete[] extensions;
}

/// <summary>
/// Creates the debug report callback.
/// </summary>
/// <param name="instance">The vk instance.</param>
/// <param name="pCreateInfo">The create information.</param>
/// <param name="pAllocator">The allocator.</param>
/// <param name="pCallback">The pointer where old callback function will be stored.</param>
/// <returns></returns>
VkResult ivk::CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
{
	auto func = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	if (func != nullptr) 
	{
		return func(instance, pCreateInfo, pAllocator, pCallback);
	}
	else 
	{
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/// <summary>
/// Destroys the debug report callback.
/// </summary>
/// <param name="instance">The vk instance.</param>
/// <param name="callback">The previous callback function.</param>
/// <param name="pAllocator">The allocator.</param>
void ivk::DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr) 
	{
		func(instance, callback, pAllocator);
	}
}

/// <summary>
/// Finds the queue families.
/// </summary>
/// <param name="device">The device.</param>
/// <returns></returns>
ivk::QueueFamilyIndices ivk::FindQueueFamilies(VkPhysicalDevice device)
{
	QueueFamilyIndices indices;

	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	for (const auto& queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);

		if (queueFamily.queueCount > 0 && 
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT && 
			presentSupport)
		{
			indices.graphicsFamily = i;
			indices.presentFamily = i;
			if (indices.isComplete()) 
			{
				break;
			}
		}

		if (queueFamily.queueCount > 0 && 
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) 
		{
			indices.graphicsFamily = i;
		}

		if (queueFamily.queueCount > 0 && 
			presentSupport && 
			indices.presentFamily == -1) 
		{
			indices.presentFamily = i;
		}

		if (queueFamily.queueCount > 0 && 
			queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT && 
			(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0)
		{
			indices.transferFamily = i;
		}

		i++;
	}

	if (indices.transferFamily == -1)
	{
		indices.transferFamily = indices.graphicsFamily;
	}

	return indices;
}

/// <summary>
/// Checks the device extension support.
/// </summary>
/// <param name="device">The device.</param>
/// <returns></returns>
bool ivk::CheckDeviceExtensionSupport(VkPhysicalDevice device)
{
	uint32_t extensionCount;
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

	std::vector<VkExtensionProperties> availableExtensions(extensionCount);
	vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

	std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

	for (const auto& extension : availableExtensions)
	{
		requiredExtensions.erase(extension.extensionName);
	}

	return requiredExtensions.empty();
}

/// <summary>
/// Queries the swap chain support.
/// </summary>
/// <param name="device">The device.</param>
/// <returns></returns>
ivk::SwapChainSupportDetails ivk::QuerySwapChainSupport(VkPhysicalDevice device)
{
	SwapChainSupportDetails details;

	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, _surface, &details.capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, nullptr);

	if (formatCount != 0) {
		details.formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, _surface, &formatCount, details.formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, nullptr);

	if (presentModeCount != 0) 
	{
		details.presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, _surface, &presentModeCount, details.presentModes.data());
	}

	return details;
}


VkSurfaceFormatKHR ivk::ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
{
	if (availableFormats.size() == 1 && 
		availableFormats[0].format == VK_FORMAT_UNDEFINED)
	{
		return { VK_FORMAT_B8G8R8A8_UNORM, VK_COLOR_SPACE_SRGB_NONLINEAR_KHR };
	}

	for (const auto& availableFormat : availableFormats) 
	{
		if (availableFormat.format == VK_FORMAT_B8G8R8A8_UNORM && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) 
		{
			return availableFormat;
		}
	}

	return availableFormats[0];
}

VkPresentModeKHR ivk::ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
{
	VkPresentModeKHR bestMode = VK_PRESENT_MODE_FIFO_KHR;

	for (const auto& availablePresentMode : availablePresentModes) 
	{
		if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) 
		{
			return availablePresentMode;
		}
		else if (availablePresentMode == VK_PRESENT_MODE_IMMEDIATE_KHR) 
		{
			bestMode = availablePresentMode;
		}
	}

	return bestMode;
}

VkExtent2D ivk::ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) 
	{
		return capabilities.currentExtent;
	}
	else 
	{
		int width, height;
		glfwGetWindowSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkImageView ivk::CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags) 
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	if (vkCreateImageView(_device, &viewInfo, _allocator, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void ivk::CreateDepthResources() 
{
	VkFormat depthFormat = FindDepthFormat();
	CreateImage(_swapChainExtent.width, _swapChainExtent.height, depthFormat, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _depthImage, _depthImageMemory);
	_depthImageView = CreateImageView(_depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);

	TransitionImageLayout(_depthImage, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
}

void ivk::CreateVertexBuffer() 
{
/*
	CreateBufferAndCopy(
		sizeof(vertices[0]) * vertices.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		vertices.data(),
		vertexBuffer,
		vertexBufferMemory);
		*/
	/*
	std::cout << "createVertexBuffer() - 1\n";

	CreateBufferAndCopy(
		sizeof(verticesPos[0]) * verticesPos.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		verticesPos.data(),
		vertexBufferPos,
		vertexBufferPosMemory);

	std::cout << "createVertexBuffer() - 2\n";

	CreateBufferAndCopy(
		sizeof(verticesColor[0]) * verticesColor.size(),
		VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		verticesColor.data(),
		vertexBufferColor,
		vertexBufferColorMemory);

	std::cout << "createVertexBuffer() - 3\n";
	*/
	VkDeviceSize bufferSize = sizeof(vertices[0]) * vertices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	std::cout << "createVertexBuffer() - 4\n";
	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
		stagingBuffer,
		stagingBufferMemory);
	std::cout << "createVertexBuffer() - 5\n";

	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, vertices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBufferMemory);
	std::cout << "createVertexBuffer() - 6\n";

	CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
		_vertexBuffer,
		_vertexBufferMemory);
	//VK_SHARING_MODE_CONCURRENT);
	std::cout << "createVertexBuffer() - 7\n";

	CopyBuffer(stagingBuffer, _vertexBuffer, bufferSize);

	vkDestroyBuffer(_device, stagingBuffer, _allocator);
	vkFreeMemory(_device, stagingBufferMemory, _allocator);
}


void ivk::CreateIndexBuffer() 
{
	VkDeviceSize bufferSize = sizeof(indices[0]) * indices.size();

	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

	void* data;
	vkMapMemory(_device, stagingBufferMemory, 0, bufferSize, 0, &data);
	memcpy(data, indices.data(), (size_t)bufferSize);
	vkUnmapMemory(_device, stagingBufferMemory);

	CreateBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, _indexBuffer, _indexBufferMemory);

	CopyBuffer(stagingBuffer, _indexBuffer, bufferSize);

	vkDestroyBuffer(_device, stagingBuffer, _allocator);
	vkFreeMemory(_device, stagingBufferMemory, _allocator);
}

void ivk::CreateUniformBuffer() 
{
	VkDeviceSize bufferSize = sizeof(UniformBufferObject);
	CreateBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, _uniformBuffer, _uniformBufferMemory);
}

void ivk::CreateDescriptorPool()
{
//	std::array<VkDescriptorPoolSize, 2> poolSizes = {};
//	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
//	poolSizes[0].descriptorCount = 1;
//	poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
//	poolSizes[1].descriptorCount = 1;

	std::array<VkDescriptorPoolSize, 1> poolSizes = {};
	poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	poolSizes[0].descriptorCount = 1;

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = 1;

	if (vkCreateDescriptorPool(_device, &poolInfo, _allocator, &_descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

// ------------------------------------------------------------------------

void ivk::CreateDescriptorSet() 
{
	VkDescriptorSetLayout layouts[] = { _descriptorSetLayout };
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = _descriptorPool;
	allocInfo.descriptorSetCount = 1;
	allocInfo.pSetLayouts = layouts;

	if (vkAllocateDescriptorSets(_device, &allocInfo, &_descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	VkDescriptorBufferInfo bufferInfo = {};
	bufferInfo.buffer = _uniformBuffer;
	bufferInfo.offset = 0;
	bufferInfo.range = sizeof(UniformBufferObject);
/*
	VkDescriptorImageInfo imageInfo = {};
	imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	imageInfo.imageView = _textureImageView;
	imageInfo.sampler = _textureSampler;
*/

/*	std::array<VkWriteDescriptorSet, 2> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = _descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[1].dstSet = _descriptorSet;
	descriptorWrites[1].dstBinding = 1;
	descriptorWrites[1].dstArrayElement = 0;
	descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
	descriptorWrites[1].descriptorCount = 1;
	descriptorWrites[1].pImageInfo = &imageInfo;
	*/
	std::array<VkWriteDescriptorSet, 1> descriptorWrites = {};

	descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrites[0].dstSet = _descriptorSet;
	descriptorWrites[0].dstBinding = 0;
	descriptorWrites[0].dstArrayElement = 0;
	descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	descriptorWrites[0].descriptorCount = 1;
	descriptorWrites[0].pBufferInfo = &bufferInfo;

	vkUpdateDescriptorSets(_device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
}

void ivk::CreateCommandBuffers() 
{
	_commandBuffers.resize(_swapChainFramebuffers.size());

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = _commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)_commandBuffers.size();

	if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}

	for (size_t i = 0; i < _commandBuffers.size(); i++) {
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
		beginInfo.pInheritanceInfo = nullptr; // Optional

		vkBeginCommandBuffer(_commandBuffers[i], &beginInfo);

		VkRenderPassBeginInfo renderPassInfo = {};
		renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
		renderPassInfo.renderPass = _renderPass;
		renderPassInfo.framebuffer = _swapChainFramebuffers[i];

		renderPassInfo.renderArea.offset = { 0, 0 };
		renderPassInfo.renderArea.extent = _swapChainExtent;

		std::array<VkClearValue, 2> clearValues = {};
		clearValues[0].color = { 0.1f, 0.1f, 0.1f, 1.0f };
		clearValues[1].depthStencil = { 1.0f, 0 };
		renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
		renderPassInfo.pClearValues = clearValues.data();

		vkCmdBeginRenderPass(_commandBuffers[i], &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

		vkCmdBindPipeline(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

		vkCmdBindDescriptorSets(_commandBuffers[i], VK_PIPELINE_BIND_POINT_GRAPHICS, _pipelineLayout, 0, 1, &_descriptorSet, 0, nullptr);

		VkBuffer vertexBuffers[] = { _vertexBuffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(_commandBuffers[i], 0, 1, vertexBuffers, offsets);
//		vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		vkCmdBindIndexBuffer(_commandBuffers[i], _indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		/*
		switch (vertexBufferMode)
		{
		case ONE_BUFFER_ONE_BINDING:
		{
			VkBuffer vertexBuffers[] = { vertexBuffer };
			VkDeviceSize offsets[] = { 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 1, vertexBuffers, offsets);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT32);
		}
		break;

		case TWO_BUFFERS_TWO_BINDINGS:
		{
			VkBuffer vertexBuffers2[] = { vertexBufferPos, vertexBufferColor };
			VkDeviceSize offsets2[] = { 0, 0 };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 2, vertexBuffers2, offsets2);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		}
		break;

		case ONE_BUFFER_TWO_BINDINGS:
		{
			VkBuffer vertexBuffers2[] = { vertexBuffer, vertexBuffer };
			VkDeviceSize offsets2[] = { offsetof(Vertex, pos), offsetof(Vertex, color) };
			vkCmdBindVertexBuffers(commandBuffers[i], 0, 2, vertexBuffers2, offsets2);
			vkCmdBindIndexBuffer(commandBuffers[i], indexBuffer, 0, VK_INDEX_TYPE_UINT16);
		}
		break;

		}
		*/
		vkCmdDrawIndexed(_commandBuffers[i], static_cast<uint32_t>(indices.size()), 1, 0, 0, 0);

		vkCmdEndRenderPass(_commandBuffers[i]);

		if (vkEndCommandBuffer(_commandBuffers[i]) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void ivk::CreateSemaphores() 
{
	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(_device, &semaphoreInfo, _allocator, &_imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(_device, &semaphoreInfo, _allocator, &_renderFinishedSemaphore) != VK_SUCCESS) {

		throw std::runtime_error("failed to create semaphores!");
	}
}

VkCommandBuffer ivk::BeginSingleTimeCommands() 
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = _commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void ivk::EndSingleTimeCommands(VkCommandBuffer commandBuffer) 
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(_graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_graphicsQueue);

	vkFreeCommandBuffers(_device, _commandPool, 1, &commandBuffer);
}

void ivk::TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout) 
{
	VkCommandBuffer commandBuffer = BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	}
	else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}


	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	}
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	}
	else {
		throw std::invalid_argument("unsupported layout transition!");
	}

	vkCmdPipelineBarrier(
		commandBuffer,
		sourceStage, destinationStage,
		0,
		0, nullptr,
		0, nullptr,
		1, &barrier
	);

	EndSingleTimeCommands(commandBuffer);
}

void ivk::CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) 
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
//		allocInfo.commandPool = commandPool;
	allocInfo.commandPool = _transferPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(_device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0; // Optional
	copyRegion.dstOffset = 0; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

//		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
//		vkQueueWaitIdle(graphicsQueue);
	vkQueueSubmit(_transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(_transferQueue);

	vkFreeCommandBuffers(_device, allocInfo.commandPool, 1, &commandBuffer);
}

void ivk::CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void *srcData, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	CreateBuffer(size, usage, properties, buffer, bufferMemory);

	void* data;
	vkMapMemory(_device, bufferMemory, 0, size, 0, &data);
	memcpy(data, srcData, (size_t)size);
	vkUnmapMemory(_device, bufferMemory);
}

void ivk::CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode) 
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;
	if (sharingMode == VK_SHARING_MODE_CONCURRENT)
	{
		QueueFamilyIndices indices = FindQueueFamilies(_physicalDevice);
		uint32_t sharedQueueInexes[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.transferFamily) };
		bufferInfo.queueFamilyIndexCount = 2;
		bufferInfo.pQueueFamilyIndices = sharedQueueInexes;
	}

	if (vkCreateBuffer(_device, &bufferInfo, _allocator, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(_device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &allocInfo, _allocator, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(_device, buffer, bufferMemory, 0);
}

void ivk::CreateImage(uint32_t width, uint32_t height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = 1;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(_device, &imageInfo, _allocator, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(_device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(_device, &allocInfo, _allocator, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(_device, image, imageMemory, 0);
}

uint32_t ivk::FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(_physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

VkFormat ivk::FindDepthFormat()
{
	return FindSupportedFormat(
	{ VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT },
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

bool ivk::HasStencilComponent(VkFormat format) 
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

VkFormat ivk::FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(_physicalDevice, format, &props);
		if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features)
		{
			return format;
		}
	}

	throw std::runtime_error("failed to find supported format!");
}

// ============================================================================

/// <summary>
///  
/// </summary>
/// <seealso cref="IExternalModule{RenderingModule, RENDERING_ENGINE}" />
class RenderingModule : public IExternalModule<RenderingModule, RENDERING_ENGINE>
{

public:
	RenderingModule() {}

	void Initialize() 
	{ 
		glwf.Init();
		vk.Init();
		U32 extensionCount = 0;
		vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
	}

	void Finalize() { 
		vk.Cleanup();
		glwf.Cleanup();
	}

	void Update(float dt) 
	{ 
		glwf.Update(dt);
		if (glwf.main3Dwindow) {
			vk.Update(dt);
		}
	}

	void SendMsg(Message *msg) 
	{ 
		switch (msg->id)
		{
		case CREATE_3D_WINDOW:
		{
			int width = 800, height = 600;
			glwf.Create_main3dwindow(width, height);
			vk.Create_main3dwindow();
		}
			break;
		}
	}

	void Destroy() {}
};


RenderingModule RM;

extern "C" {
	REVK_EXPORT void RenderingEngine_RegisterModule()
	{
		printf("HELLO REVK\n");
		RM.Register();
		
	}

}