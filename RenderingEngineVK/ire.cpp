#include "stdafx.h"


#include <chrono>
#include <glm/gtc/matrix_transform.hpp>

// ============================================================================

const std::vector<const char*> ire::validationLayers = { "VK_LAYER_LUNARG_standard_validation" };
const std::vector<const char*> ire::deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME };
bool ire::enableValidationLayers = false;

ire re;
// ============================================================================ SwapChainSupportDetails ===

SwapChainSupportDetails::SwapChainSupportDetails(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &capabilities);

	uint32_t formatCount;
	vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

	if (formatCount != 0) {
		formats.resize(formatCount);
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, formats.data());
	}

	uint32_t presentModeCount;
	vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

	if (presentModeCount != 0)
	{
		presentModes.resize(presentModeCount);
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, presentModes.data());
	}
}

// ============================================================================ QueueFamilyIndices ===

QueueFamilyIndices::QueueFamilyIndices(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	uint32_t queueFamilyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

	std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
	vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

	int i = 0;
	int backup_transferFamily = -1;
	for (const auto& queueFamily : queueFamilies)
	{
		VkBool32 presentSupport = false;
		vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

		if (queueFamily.queueCount > 0 &&
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
			presentSupport)
		{
			graphicsFamily = i;
			presentFamily = i;

			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
				backup_transferFamily = i;
			}

			if (isComplete()) {
				break;
			}
		}

		// allow to select 2 different queues, one for graphics, one for present (only if we don't have one with both)
		if (queueFamily.queueCount > 0 &&
			queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT &&
			graphicsFamily == -1)
		{
			graphicsFamily = i;
			if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
				backup_transferFamily == -1) {
				backup_transferFamily = i;
			}
		}

		if (queueFamily.queueCount > 0 &&
			presentSupport &&
			presentFamily == -1)
		{
			presentFamily = i;
		}

		if (queueFamily.queueCount > 0 &&
			queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT &&
			(queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) == 0) // we want different transfer queue than graphics
		{
			transferFamily = i;
		}

		i++;
	}

	if (transferFamily == -1) // in case if we don't have separate transfer queue, use graphics queue (it will have 
	{
		transferFamily = backup_transferFamily;
	}
}


// ============================================================================ ire ===

/// <summary>
/// Initializes VK instance.
/// All common structores for all windows.
/// ... but no window creation, no framebuffers, no command lists, no data.
/// </summary>
void ire::Init()
{
	CreateInstance();
	SetupDebugCallback();
	for (auto &ow : outputWindows)
		ow = new OutputWindow();
}

/// <summary>
/// Creates vulkan instance.
/// [Init. Step 1.]
/// </summary>
void ire::CreateInstance()
{
#ifdef _DEBUG
	enableValidationLayers = true;
#endif

	_ListVKExtensions(std::cout);
	if (enableValidationLayers && !_IsValidationLayerSupported())
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

	auto extensions = _GetRequiredExtensions();
	auto layers = _GetRequiredLayers();

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
void ire::SetupDebugCallback()
{
	if (enableValidationLayers)
	{

		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = _DebugCallback;

		if (_CreateDebugReportCallbackEXT(_instance, &createInfo, _allocator, &_callback) != VK_SUCCESS)
		{
			throw std::runtime_error("failed to set up debug callback!");
		}


		{
			VkDebugUtilsMessengerCreateInfoEXT createInfo = {};
			createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
			createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
			createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
			createInfo.pfnUserCallback = _DebugCallback2;
			createInfo.pUserData = nullptr; // Optional

			if (_CreateDebugUtilsMessengerEXT(_instance, &createInfo, nullptr, &_debugMessengerCallback) != VK_SUCCESS) {
				throw std::runtime_error("failed to set up debug messenger!");
			}
		}
	}
}

// ============================================================================

void ire::Cleanup()
{
	if (_instance)
	{
		for (auto &ow : outputWindows)
		{
			if (ow)
			{
				delete ow;
				ow = nullptr;
			}
		}

		if (_callback)
			_DestroyDebugReportCallbackEXT(_instance, _callback, _allocator);

		if (_debugMessengerCallback)
			_DestroyDebugUtilsMessengerEXT(_instance, _debugMessengerCallback, _allocator);

		vkDestroyInstance(_instance, _allocator);

		_callback = VK_NULL_HANDLE;
		_instance = VK_NULL_HANDLE;
	}
}

void ire::Update(float dt)
{
	for (auto &ow : outputWindows)
	{
		if (ow && ow->IsValid()) 
		{
			ow->UpdateUniformBuffer();
			ow->DrawFrame();
		}
	}
}

void ire::CreateWnd(const void * params)
{
	auto p = static_cast<const PCREATE_WINDOW *>(params);
	if (!p || p->wnd >= COUNT_OF(outputWindows))
		return;

	GLFWwindow *window = glfw.CreateWnd(p->wnd, p->w, p->h);
	glfwSetWindowPos(window, p->x, p->y);

	auto ow = outputWindows[p->wnd];
	ow->Prepare(_instance, window, _allocator);
}

void ire::CloseWnd(GLFWwindow* window)
{
	for (auto &ow : outputWindows)
	{
		ow->Close(window);
	}
}

void ire::CloseWnd(const void * params)
{
	auto p = static_cast<const PCLOSE_WINDOW *>(params);
	if (!p || p->wnd >= COUNT_OF(outputWindows))
		return;

	auto ow = outputWindows[p->wnd];
	ow->Close();
}

#define CASTMSGPARAMS(x) \
auto p = static_cast<const x *>(params);\
if (!p || p->wnd >= COUNT_OF(outputWindows)) return; \
auto ow = outputWindows[p->wnd]; \
if (!ow || !ow->Exist())  return;


/// <summary>
/// Adds the mesh.
/// </summary>
/// <param name="params">The parameters.</param>
void  ire::AddMesh(const void * params)
{
	CASTMSGPARAMS(PADD_MESH);
	auto oi = ow->AddObject(p->mesh, p->shader);
	if (oi)
	{
		if (p->pProperties)
			*p->pProperties = oi->GetProperties();
		if (p->pId)
			*p->pId = oi->oid;
	}
}

void  ire::AddModel(const void * params)
{
	CASTMSGPARAMS(PADD_MODEL);
	ow->AddModel(p->modelName);
	//auto oi = ow->AddObject(p->name, p->mesh, p->shader);
	//if (oi)
	//{
	//	if (p->pProperties)
	//		*p->pProperties = oi->GetProperties();
	//	if (p->pId)
	//		*p->pId = oi->oid;
	//}
}

void ire::AddTexture(const void * params)
{
	CASTMSGPARAMS(PADD_TEXTURE);
	ow->AddTexture(p->propVal, p->textureResourceName);
}

void ire::AddShader(const void * params)
{
	CASTMSGPARAMS(PADD_SHADER);
	ow->AddShader(p->shader);
}

void ire::ReloadShader(const void *params)
{
	auto p = static_cast<const PRELOAD_SHADER *>(params); 
	assert(p!=nullptr);
	if (p->wnd == -1)
	{
		for (auto &ow : outputWindows)
		{
			if (ow && ow->Exist())
			{
				TRACE("ReloadShader: " << p->shader << ", wnd=" << p->wnd << "\n");
				ow->ReloadShader(p->shader);
			}
		}
	}
	else if (p->wnd >= COUNT_OF(outputWindows)) 
	{
		auto ow = outputWindows[p->wnd];
		if (!ow || !ow->Exist())  
			return;

		TRACE("ReloadShader: " << p->shader << ", wnd=" << p->wnd << "\n");
		ow->ReloadShader(p->shader);

	}
}

void ire::GetShaderParams(const void * params)
{
	CASTMSGPARAMS(PGET_SHADER_PARAMS);
	ow->GetShaderParams(p->name, p->pProperties);
}

void ire::GetObjectParams(const void * params)
{
	CASTMSGPARAMS(PGET_OBJECT_PARAMS);
	ow->GetObjectParams(p->objectIdx, p->pProperties);
}

void ire::UpdateDrawCommands(const void *params)
{
	CASTMSGPARAMS(PUPDATE_DRAW_COMMANDS);
	ow->UpdateDrawCommands();
}

void ire::SetCamera(const void *params)
{
	CASTMSGPARAMS(PSET_CAMERA);
	ow->SetCamera(p);
}


// ============================================================================ ire : Rendering Engine - protected methods ===

/// <summary>
/// Vulkan debug callback function.
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
VKAPI_ATTR VkBool32 VKAPI_CALL ire::_DebugCallback(
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


VKAPI_ATTR VkBool32 VKAPI_CALL ire::_DebugCallback2(
	VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
	VkDebugUtilsMessageTypeFlagsEXT messageType,
	const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
	void* pUserData) 
{

	if (messageSeverity > VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT)
		std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

	return VK_FALSE;
}

/// <summary>
/// Creates the debug report callback.
/// </summary>
/// <param name="instance">The vk instance.</param>
/// <param name="pCreateInfo">The create information.</param>
/// <param name="pAllocator">The allocator.</param>
/// <param name="pCallback">The pointer where old callback function will be stored.</param>
/// <returns></returns>
VkResult ire::_CreateDebugReportCallbackEXT(VkInstance instance, const VkDebugReportCallbackCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugReportCallbackEXT* pCallback)
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

VkResult ire::_CreateDebugUtilsMessengerEXT(VkInstance _instance, const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, const VkAllocationCallbacks* pAllocator, VkDebugUtilsMessengerEXT* pDebugMessenger) {
	auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
	if (func != nullptr) {
		return func(_instance, pCreateInfo, pAllocator, pDebugMessenger);
	}
	else {
		return VK_ERROR_EXTENSION_NOT_PRESENT;
	}
}

/// <summary>
/// Destroys the debug report callback.
/// </summary>
/// <param name="instance">The vk instance.</param>
/// <param name="callback">The previous callback function.</param>
/// <param name="pAllocator">The allocator.</param>
void ire::_DestroyDebugReportCallbackEXT(VkInstance instance, VkDebugReportCallbackEXT callback, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
	if (func != nullptr)
	{
		func(instance, callback, pAllocator);
	}
}

void ire::_DestroyDebugUtilsMessengerEXT(VkInstance instance, VkDebugUtilsMessengerEXT debugMessenger, const VkAllocationCallbacks* pAllocator)
{
	auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
	if (func != nullptr) {
		func(instance, debugMessenger, pAllocator);
	}
}

bool ire::_HasStencilComponent(VkFormat format)
{
	return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
}

/// <summary>
/// Checks the device extension support.
/// </summary>
/// <param name="device">The device.</param>
/// <returns></returns>
bool ire::_CheckDeviceExtensionSupport(VkPhysicalDevice device)
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

VkSurfaceFormatKHR ire::_ChooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats)
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

VkPresentModeKHR ire::_ChooseSwapPresentMode(const std::vector<VkPresentModeKHR> availablePresentModes)
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

VkExtent2D ire::_ChooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities, GLFWwindow* window)
{
	if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max())
	{
		return capabilities.currentExtent;
	}
	else
	{
		int width, height;
		glfw.GetWndSize(window, &width, &height);

		VkExtent2D actualExtent = {
			static_cast<uint32_t>(width),
			static_cast<uint32_t>(height)
		};

		actualExtent.width = std::max(capabilities.minImageExtent.width, std::min(capabilities.maxImageExtent.width, actualExtent.width));
		actualExtent.height = std::max(capabilities.minImageExtent.height, std::min(capabilities.maxImageExtent.height, actualExtent.height));

		return actualExtent;
	}
}

VkSampleCountFlagBits ire::_GetMaxUsableSampleCount(VkPhysicalDevice physicalDevice)
{
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

	VkSampleCountFlags counts = std::min(physicalDeviceProperties.limits.framebufferColorSampleCounts, physicalDeviceProperties.limits.framebufferDepthSampleCounts);
	if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
	if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
	if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
	if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
	if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
	if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

	return VK_SAMPLE_COUNT_1_BIT;
}

/// <summary>
/// Lists the vulkan extensions.
/// Debug only.
/// </summary>
/// <param name="out">The out. ostream.</param>
void ire::_ListVKExtensions(std::ostream &out)
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
/// Checks the vulkan validation layer support.
/// [Init. Step 1.1.]
/// </summary>
/// <returns></returns>
bool ire::_IsValidationLayerSupported()
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
std::vector<const char*> ire::_GetRequiredExtensions()
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
		extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
	}

	return extensions;
}

/// <summary>
/// Gets the required vulkan layers (validation layer if debug is enabled).
/// </summary>
/// <returns></returns>
std::vector<const char*> ire::_GetRequiredLayers()
{
	return enableValidationLayers ? validationLayers : std::vector<const char *>();
}

