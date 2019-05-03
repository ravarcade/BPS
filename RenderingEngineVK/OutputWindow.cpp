#include "stdafx.h"
#include "stdafx.h"

#include <chrono>
#include <glm/gtc/matrix_transform.hpp>


OutputWindow::OutputWindow() :
	instance(VK_NULL_HANDLE),
	window(nullptr),
	allocator(VK_NULL_HANDLE),
	device(VK_NULL_HANDLE),
	renderPass(VK_NULL_HANDLE)
{
}

// ----------------------------------------------------------------------------

void OutputWindow::Init()
{
	instance = VK_NULL_HANDLE;
	physicalDevice = VK_NULL_HANDLE;
	surface = VK_NULL_HANDLE;

	device = VK_NULL_HANDLE;
	graphicsQueue = VK_NULL_HANDLE;
	presentQueue = VK_NULL_HANDLE;
	transferQueue = VK_NULL_HANDLE;
	renderPass = VK_NULL_HANDLE;
}

// ----------------------------------------------------------------------------

void OutputWindow::Prepare(VkInstance _instance, GLFWwindow* _window, const VkAllocationCallbacks* _allocator)
{
	Init();

	instance = _instance;
	window = _window;
	allocator = _allocator;

	glfw.SetWindowUserPointer(window, this);
	glfw.SetWindowSizeCallback(window, &OutputWindow::_OnWindowSize);

	// ------------------------------------------------------------------------ create surface in output window
	if (glfwCreateWindowSurface(instance, window, allocator, &surface) != VK_SUCCESS)
		throw std::runtime_error("failed to create window surface!");

	// ------------------------------------------------------------------------ select physical device
	uint32_t deviceCount = 0;
	vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
	if (deviceCount == 0)
		throw std::runtime_error("failed to find GPUs with Vulkan support!");

	std::vector<VkPhysicalDevice> devices(deviceCount);
	vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

	for (const auto& device : devices)
	{
		if (_IsDeviceSuitable(device))
		{
			physicalDevice = device;
//			msaaSamples = ire::_GetMaxUsableSampleCount(physicalDevice);
			msaaSamples = VK_SAMPLE_COUNT_1_BIT;
			break;
		}
	}

	if (physicalDevice == VK_NULL_HANDLE)
		throw std::runtime_error("failed to find a suitable GPU!");

	// ------------------------------------------------------------------------ create logical device
	// queues: one from graphics family one from present family (it may be same)
	QueueFamilyIndices indices(physicalDevice, surface);
	std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
	std::set<int> uniqueQueueFamilies = { indices.graphicsFamily, indices.presentFamily, indices.transferFamily }; // "std::set<int>" here will remove repeated values, so instead { 0, 0, 1 } we will have {0, 1}

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
	createInfo.enabledExtensionCount = static_cast<uint32_t>(ire::deviceExtensions.size());
	createInfo.ppEnabledExtensionNames = ire::deviceExtensions.data();

	// add validation layer for logical device
	if (ire::enableValidationLayers)
	{
		createInfo.enabledLayerCount = static_cast<uint32_t>(ire::validationLayers.size());
		createInfo.ppEnabledLayerNames = ire::validationLayers.data();
	}
	else {
		createInfo.enabledLayerCount = 0;
	}

	if (vkCreateDevice(physicalDevice, &createInfo, _allocator, &device) != VK_SUCCESS)
		throw std::runtime_error("failed to create logical device!");

	vkGetDeviceQueue(device, indices.graphicsFamily, 0, &graphicsQueue);
	vkGetDeviceQueue(device, indices.presentFamily, 0, &presentQueue);
	vkGetDeviceQueue(device, indices.transferFamily, 0, &transferQueue);

	// ------------------------------------------------------------------------ create command pool
	{
		QueueFamilyIndices queueFamilyIndices(physicalDevice, surface);

		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily;
		poolInfo.flags = 0; // Optional

		if (vkCreateCommandPool(device, &poolInfo, _allocator, &commandPool) != VK_SUCCESS) {
			throw std::runtime_error("failed to create command pool!");
		}

		if (queueFamilyIndices.graphicsFamily != queueFamilyIndices.transferFamily)
		{
			VkCommandPoolCreateInfo poolInfo = {};
			poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
			poolInfo.queueFamilyIndex = queueFamilyIndices.transferFamily;
			poolInfo.flags = 0; // Optional

			if (vkCreateCommandPool(device, &poolInfo, _allocator, &transferPool) != VK_SUCCESS) {
				throw std::runtime_error("failed to create command pool!");
			}

		}
	}

	// ------------------------------------------------------------------------ create semaphores

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(device, &semaphoreInfo, allocator, &imageAvailableSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, allocator, &renderFinishedSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores!");
	}

	// ------------------------------------------------------------------------ 

	_CreateRenderPass(); // we must call it before we create swap chain. Swap chain will use one renderpass to present image
	if (_CreateSwapChain())
	{
		_CreateSharedUniform(); // we need to create shared uniform before we call _LoadShadersPrograms?
		_LoadShaderPrograms();
		_CreateCommandBuffers();
	}
}

VkDescriptorSet OutputWindow::CreateDescriptorSets(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts)
{
	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	allocInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkAllocateDescriptorSets(device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}
	
	return descriptorSet;
}

void OutputWindow::Close(GLFWwindow* wnd)
{
	if (wnd) {
		if (wnd == window)
		{
			_Cleanup();
			window = nullptr;
		}
		return;
	}

	if (window) {
		glfwSetWindowShouldClose(window, GLFW_TRUE);
	}
	else {
		TRACE("HM");
	}
}

void OutputWindow::_OnWindowSize(GLFWwindow *wnd, int width, int height)
{
	auto *ow = reinterpret_cast<OutputWindow*>(glfw.GetWindowUserPointer(wnd));
	ow->resizeWindow = true;
}

bool OutputWindow::_IsDeviceSuitable(VkPhysicalDevice device)
{
	VkPhysicalDeviceProperties deviceProperties;
	VkPhysicalDeviceFeatures deviceFeatures;

	vkGetPhysicalDeviceProperties(device, &deviceProperties);
	vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

	QueueFamilyIndices indices(device, surface);

	bool extensionsSupported = ire::_CheckDeviceExtensionSupport(device);

	bool swapChainAdequate = false;
	if (extensionsSupported)
	{
		SwapChainSupportDetails swapChainSupport(device, surface);
		swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
	}

	return deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU &&
		//		deviceFeatures.geometryShader &&
		indices.isComplete() &&
		extensionsSupported &&
		deviceFeatures.samplerAnisotropy &&
		swapChainAdequate;
}

// ----------------------------------------------------------------------------

VkExtent2D OutputWindow::_GetVkExtentSize()
{
	VkSurfaceCapabilitiesKHR capabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &capabilities);
	VkExtent2D extent = ire::_ChooseSwapExtent(capabilities, window);

	return extent;
}

// ----------------------------------------------------------------------------

bool OutputWindow::_CreateSwapChain()

{
	// ------------------------------------------------------------------------ create swap chain
	SwapChainSupportDetails swapChainSupport(physicalDevice, surface);
	VkExtent2D extent = ire::_ChooseSwapExtent(swapChainSupport.capabilities, window);

	VkSurfaceFormatKHR surfaceFormat = ire::_ChooseSwapSurfaceFormat(swapChainSupport.formats);
	VkPresentModeKHR presentMode = ire::_ChooseSwapPresentMode(swapChainSupport.presentModes);
	if (extent.height == 0 || extent.width == 0)
		return false;

	uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
	if (swapChainSupport.capabilities.maxImageCount > 0 &&
		imageCount > swapChainSupport.capabilities.maxImageCount)
	{
		imageCount = swapChainSupport.capabilities.maxImageCount;
	}

	VkSwapchainCreateInfoKHR createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	createInfo.surface = surface;

	createInfo.minImageCount = imageCount;
	createInfo.imageFormat = surfaceFormat.format;
	createInfo.imageColorSpace = surfaceFormat.colorSpace;
	createInfo.imageExtent = extent;
	createInfo.imageArrayLayers = 1;
	createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // VK_IMAGE_USAGE_TRANSFER_DST_BIT 

	QueueFamilyIndices indices(physicalDevice, surface);
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

	if (vkCreateSwapchainKHR(device, &createInfo, allocator, &swapChain) != VK_SUCCESS) {
		throw std::runtime_error("failed to create swap chain!");
	}

	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
	swapChainImages.resize(imageCount);
	vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

	swapChainImageFormat = surfaceFormat.format;
	swapChainExtent = extent;

	// ------------------------------------------------------------------------ create swap chaing image views
	swapChainImageViews.resize(swapChainImages.size());

	for (size_t i = 0; i < swapChainImages.size(); i++) {
		swapChainImageViews[i] = _CreateImageView(swapChainImages[i], swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT);
	}

	// ------------------------------------------------------------------------ create render pass

	//	if (renderPass == nullptr)
	//		_CreateSimpleRenderPass(swapChainImageFormat, msaaSamples);

	// ------------------------------------------------------------------------ create color, depth resources

	_CreateAttachment(swapChainImageFormat, VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, swapChainExtent, color);
	_CreateAttachment(_FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, swapChainExtent, depth);

	// ------------------------------------------------------------------------ create frame buffers
	swapChainFramebuffers.resize(swapChainImageViews.size());

	for (size_t i = 0; i < swapChainImageViews.size(); i++)
	{
		std::vector<VkImageView> attachments = { swapChainImageViews[i], depth.view };
		if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
			attachments = { color.view, depth.view,  swapChainImageViews[i] };

		VkFramebufferCreateInfo framebufferInfo = {};
		framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferInfo.renderPass = renderPass;
		framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
		framebufferInfo.pAttachments = attachments.data();
		framebufferInfo.width = swapChainExtent.width;
		framebufferInfo.height = swapChainExtent.height;
		framebufferInfo.layers = 1;

		if (vkCreateFramebuffer(device, &framebufferInfo, allocator, &swapChainFramebuffers[i]) != VK_SUCCESS)
			throw std::runtime_error("failed to create framebuffer!");
	}

	resizeWindow = false;
	return true;
}


VkFormat OutputWindow::_FindDepthFormat()
{
	return _FindSupportedFormat(
		{ VK_FORMAT_D24_UNORM_S8_UINT, /*VK_FORMAT_D32_SFLOAT,*/ VK_FORMAT_D32_SFLOAT_S8_UINT }, // i want stencil buffer?
//		{ VK_FORMAT_D24_UNORM_S8_UINT, VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT }, // i want stencil buffer?
		VK_IMAGE_TILING_OPTIMAL,
		VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT
	);
}

VkFormat OutputWindow::_FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (VkFormat format : candidates)
	{
		VkFormatProperties props;
		vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &props);
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

// ============================================================================ Image (textures) ===
/// <summary>
/// Creates the image.
/// </summary>
/// <param name="width">The width.</param>
/// <param name="height">The height.</param>
/// <param name="mipLevels">The mip levels.</param>
/// <param name="numSamples">Liczba próbek</param>
/// <param name="format">The format.</param>
/// <param name="tiling">The tiling.</param>
/// <param name="usage">The usage.</param>
/// <param name="properties">The properties.</param>
/// <param name="image">The image.</param>
/// <param name="imageMemory">The image memory.</param>
void OutputWindow::_CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory)
{
	VkImageCreateInfo imageInfo = {};
	imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageInfo.imageType = VK_IMAGE_TYPE_2D;
	imageInfo.extent.width = width;
	imageInfo.extent.height = height;
	imageInfo.extent.depth = 1;
	imageInfo.mipLevels = mipLevels;
	imageInfo.arrayLayers = 1;
	imageInfo.format = format;
	imageInfo.tiling = tiling;
	imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageInfo.usage = usage;
	imageInfo.samples = numSamples;
	imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	if (vkCreateImage(device, &imageInfo, allocator, &image) != VK_SUCCESS) {
		throw std::runtime_error("failed to create image!");
	}

	VkMemoryRequirements memRequirements;
	vkGetImageMemoryRequirements(device, image, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = _FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, allocator, &imageMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate image memory!");
	}

	vkBindImageMemory(device, image, imageMemory, 0);
}

VkImageView OutputWindow::_CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
	VkImageViewCreateInfo viewInfo = {};
	viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
	viewInfo.image = image;
	viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
	viewInfo.format = format;
	viewInfo.components = {
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY,
		VK_COMPONENT_SWIZZLE_IDENTITY };
	viewInfo.subresourceRange.aspectMask = aspectFlags;
	viewInfo.subresourceRange.baseMipLevel = 0;
	viewInfo.subresourceRange.levelCount = VK_REMAINING_MIP_LEVELS; // 1;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = VK_REMAINING_ARRAY_LAYERS; // 1;

	VkImageView imageView;
	if (vkCreateImageView(device, &viewInfo, allocator, &imageView) != VK_SUCCESS) {
		throw std::runtime_error("failed to create texture image view!");
	}

	return imageView;
}

void OutputWindow::_CreateAttachment(VkFormat format, VkImageUsageFlags usage, VkExtent2D extent, FrameBufferAttachment &attachment)
{
	VkImageAspectFlags aspectFlags = 0;
	VkImageLayout imageLayout;
	uint32_t mipLevels = 1;
	VkSampleCountFlagBits numSamples = msaaSamples;
	VkImageTiling tiling = VK_IMAGE_TILING_OPTIMAL;
	VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

	attachment.format = format;
	attachment.usage = usage;
	if (usage & VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT)
	{
//		usage |= VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT;
		aspectFlags = VK_IMAGE_ASPECT_COLOR_BIT;
		imageLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}
	if (usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
	{
		aspectFlags = VK_IMAGE_ASPECT_DEPTH_BIT | VK_IMAGE_ASPECT_STENCIL_BIT;
		imageLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	_CreateImage(extent.width, extent.height, mipLevels, numSamples, format, tiling, usage, properties, attachment.image, attachment.memory);
	attachment.view = _CreateImageView(attachment.image, format, aspectFlags);
	_TransitionImageLayout(attachment.image, format, VK_IMAGE_LAYOUT_UNDEFINED, imageLayout);

}

/// <summary>
/// Transitions the image layout.
/// </summary>
/// <param name="image">The image.</param>
/// <param name="format">The format.</param>
/// <param name="oldLayout">The old layout.</param>
/// <param name="newLayout">The new layout.</param>
void OutputWindow::_TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = _BeginSingleTimeCommands();

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;

	barrier.image = image;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;

		if (ire::_HasStencilComponent(format)) {
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
	else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
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

	_EndSingleTimeCommands(commandBuffer);
}

// ===========================================================================

uint32_t OutputWindow::_FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties;
	vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memProperties);

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	throw std::runtime_error("failed to find suitable memory type!");
}

// ============================================================================

VkCommandBuffer OutputWindow::_BeginSingleTimeCommands()
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void OutputWindow::_EndSingleTimeCommands(VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(graphicsQueue);

	vkFreeCommandBuffers(device, commandPool, 1, &commandBuffer);
}

// ============================================================================ Buffer ===

void OutputWindow::_CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory, VkSharingMode sharingMode)
{
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = size;
	bufferInfo.usage = usage;
	bufferInfo.sharingMode = sharingMode;
	if (sharingMode == VK_SHARING_MODE_CONCURRENT)
	{
		QueueFamilyIndices indices(physicalDevice, surface);
		uint32_t sharedQueueInexes[] = { static_cast<uint32_t>(indices.graphicsFamily), static_cast<uint32_t>(indices.transferFamily) };
		bufferInfo.queueFamilyIndexCount = 2;
		bufferInfo.pQueueFamilyIndices = sharedQueueInexes;
	}

	if (vkCreateBuffer(device, &bufferInfo, allocator, &buffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to create buffer!");
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device, buffer, &memRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = _FindMemoryType(memRequirements.memoryTypeBits, properties);

	if (vkAllocateMemory(device, &allocInfo, allocator, &bufferMemory) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate buffer memory!");
	}

	vkBindBufferMemory(device, buffer, bufferMemory, 0);
}


void OutputWindow::_CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void *srcData, VkBuffer& buffer, VkDeviceMemory& bufferMemory)
{
	_CreateBuffer(size, usage, properties, buffer, bufferMemory);

	void* data;
	vkMapMemory(device, bufferMemory, 0, size, 0, &data);
	memcpy(data, srcData, (size_t)size);
	vkUnmapMemory(device, bufferMemory);
}

void OutputWindow::_CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset, VkDeviceSize dstOffset)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = transferPool; // = commandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = srcOffset; // Optional
	copyRegion.dstOffset = dstOffset; // Optional
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	//		vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	//		vkQueueWaitIdle(graphicsQueue);
	vkQueueSubmit(transferQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(transferQueue);

	vkFreeCommandBuffers(device, allocInfo.commandPool, 1, &commandBuffer);
}

VkShaderModule OutputWindow::_CreateShaderModule(const char *shaderName)
{
	BAMS::CResourceManager rm;
	auto code = rm.GetRawData(shaderName);
	if (!code.IsLoaded())
		rm.LoadSync();

	VkShaderModuleCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = code.GetSize();
	createInfo.pCode = reinterpret_cast<const uint32_t*>(code.GetData());

	VkShaderModule shaderModule;
	if (vkCreateShaderModule(device, &createInfo, allocator, &shaderModule) != VK_SUCCESS) {
		throw std::runtime_error("failed to create shader module!");
	}

	return shaderModule;
}


// ----------------------------------------------------------------------------

void OutputWindow::_Cleanup()
{
	if (device != VK_NULL_HANDLE)
	{
		_CleanupShaderPrograms();
		// do nothing for: graphicsQueue, presentQueue, transferQueue;
		_CleanupSharedUniform();
		_CleanupSwapChain();
		_CleanupRenderPass();

		vkDestroy(descriptorPool);

		vkDestroy(renderFinishedSemaphore);
		vkDestroy(imageAvailableSemaphore);

		vkDestroy(transferPool);
		vkDestroy(commandPool);

		vkDestroy(device);
		// do nothing for: physicalDevice

		vkDestroy(surface);
	}
	shaders.clear();
	objects.clear();
}

void OutputWindow::_CleanupSwapChain()
{
	// use template method vkDestroy(val).
	// it will call vkDestroy_something(device, val, allocator);
	// ... and will set val = nullptr;
	// same about vkFree

	vkDestroy(color);
	vkDestroy(depth);

	vkFree(commandPool, commandBuffers);

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
		vkDestroy(swapChainFramebuffers[i]);
	}

	for (size_t i = 0; i < swapChainImageViews.size(); i++) {
		vkDestroy(swapChainImageViews[i]);
	}

	vkDestroy(swapChain);
}

void OutputWindow::_CreateSharedUniform()
{
	VkDeviceSize bufferSize = sizeof(SharedUniformBufferObject);
	VkMemoryPropertyFlags memoryProperties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	_CreateBuffer(
		bufferSize,
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		memoryProperties,
		sharedUniformBuffer, sharedUniformBufferMemory);

	vkMapMemory(device, sharedUniformBufferMemory, 0, sizeof(bufferSize), 0, (void **)&sharedUboData);
}

void OutputWindow::_CleanupSharedUniform()
{
	if (sharedUboData) {
		vkUnmapMemory(device, sharedUniformBufferMemory);
		sharedUboData = nullptr;
	}

	vkDestroy(sharedUniformBuffer);
	vkFree(sharedUniformBufferMemory);
}

void OutputWindow::_CreateRenderPass()
{
	SwapChainSupportDetails swapChainSupport(physicalDevice, surface);
	VkSurfaceFormatKHR surfaceFormat = ire::_ChooseSwapSurfaceFormat(swapChainSupport.formats);
	_CreateSimpleRenderPass(surfaceFormat.format, msaaSamples);
	_CreateDeferredRenderPass(surfaceFormat.format, msaaSamples);
}

void OutputWindow::_CleanupRenderPass()
{
	vkDestroy(deferredFrameBuf);
	vkDestroy(renderPass);	
}

void OutputWindow::_CreateDescriptorPool()
{
	std::vector<uint32_t> pools;
	uint32_t s = 0;
	uint32_t numShaderPrograms = 0;
	shaders.foreach([&](CShaderProgram *&sh) {
		s = sh->GetDescriptorPoolsSize(pools);
		++numShaderPrograms;
	});

	if (true)  // force add 5 descriptors to pool
	{
		for (auto &x : pools)
			x += 5;
		numShaderPrograms += 5;
		s = static_cast<uint32_t>(pools.size());
	}

	std::vector<VkDescriptorPoolSize> poolSizes(s);
	for (uint32_t i = 0; i < pools.size(); ++i) {
		if (pools[i]) {
			poolSizes[i].type = static_cast<VkDescriptorType>(VK_DESCRIPTOR_TYPE_BEGIN_RANGE + i);
			poolSizes[i].descriptorCount = pools[i];
		}
	}

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
	poolInfo.pPoolSizes = poolSizes.data();
	poolInfo.maxSets = numShaderPrograms;

	if (vkCreateDescriptorPool(device, &poolInfo, allocator, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

// ------------------------------------------------------------------------

void OutputWindow::_CreateDeferredCommandBuffer(VkCommandBuffer &cb)
{
	std::array<VkClearValue, 4> clearValues = {};
	clearValues[0].color = { 0.5f, 0.5f, 0.0f, 0.0f };
	clearValues[1].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[2].color = { 0.0f, 0.0f, 0.0f, 0.0f };
	clearValues[3].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = deferredFrameBuf.renderPass;
	renderPassInfo.framebuffer = deferredFrameBuf.frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = { deferredFrameBuf.width, deferredFrameBuf.height };
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(cb, 0, 1, &viewport);
	vkCmdSetScissor(cb, 0, 1, &scissor);

	for (auto &shader : deferredFrameBuf.shaders) 
	{
		shader->DrawObjects(cb);
	}

	vkCmdEndRenderPass(cb);
}

void OutputWindow::_CreateForwardCommandBuffer(VkCommandBuffer &cb, VkFramebuffer &frameBuffer)
{
	// clear values
	std::array<VkClearValue, 2> clearValues = {};
	clearValues[0].color = { 0.2f, 0.2f, 0.2f, 1.0f };
	clearValues[1].depthStencil = { 1.0f, 0 };

	VkRenderPassBeginInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassInfo.renderPass = renderPass;
	renderPassInfo.framebuffer = frameBuffer;

	renderPassInfo.renderArea.offset = { 0, 0 };
	renderPassInfo.renderArea.extent = swapChainExtent;
	renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
	renderPassInfo.pClearValues = clearValues.data();

	vkCmdBeginRenderPass(cb, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
	vkCmdSetViewport(cb, 0, 1, &viewport);
	vkCmdSetScissor(cb, 0, 1, &scissor);

	for (auto &shader : forwardRenderingShaders)
	{
		shader->DrawObjects(cb);
	}

	vkCmdEndRenderPass(cb);
}

// ------------------------------------------------------------------------

void OutputWindow::_CreateCommandBuffers()
{
	// allocate command buffero (one for every swap-chain-frame-buffer)
	commandBuffers.resize(swapChainFramebuffers.size()+1);
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = commandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = (uint32_t)commandBuffers.size();

	if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate command buffers!");
	}	

	auto &deferredCommandBuffer = commandBuffers[swapChainFramebuffers.size()];

	// viewport
	scissor = { { 0, 0 }, swapChainExtent };
	viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	viewport.width = (float)swapChainExtent.width;
	viewport.height = (float)swapChainExtent.height;

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
	beginInfo.pInheritanceInfo = nullptr; // Optional

	currentDescriptorSet = nullptr;

	vkBeginCommandBuffer(deferredCommandBuffer, &beginInfo);
	_CreateDeferredCommandBuffer(deferredCommandBuffer);
	if (vkEndCommandBuffer(deferredCommandBuffer) != VK_SUCCESS) {
		throw std::runtime_error("failed to record command buffer!");
	}

	for (size_t i = 0; i < swapChainFramebuffers.size(); i++)
	{
		auto &cb = commandBuffers[i];
		currentDescriptorSet = nullptr;
		vkBeginCommandBuffer(cb, &beginInfo);
		_CreateForwardCommandBuffer(cb, swapChainFramebuffers[i]);
		if (vkEndCommandBuffer(cb) != VK_SUCCESS) {
			throw std::runtime_error("failed to record command buffer!");
		}
	}
}

void OutputWindow::_CreateSimpleRenderPass(VkFormat format, VkSampleCountFlagBits samples)
{
	// create render passes
	VkAttachmentDescription colorAttachment = {};
	colorAttachment.format = format;
	colorAttachment.samples = samples;
	colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachment.finalLayout = msaaSamples == VK_SAMPLE_COUNT_1_BIT ? VK_IMAGE_LAYOUT_PRESENT_SRC_KHR : VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription depthAttachment = {};
	depthAttachment.format = _FindDepthFormat();
	depthAttachment.samples = samples;
	depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentDescription colorAttachmentResolve = {};
	colorAttachmentResolve.format = format;
	colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference colorAttachmentResolveRef = {};
	colorAttachmentResolveRef.attachment = 2;
	colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.colorAttachmentCount = 1;
	subpass.pColorAttachments = &colorAttachmentRef;
	subpass.pDepthStencilAttachment = &depthAttachmentRef;
	subpass.pResolveAttachments = msaaSamples != VK_SAMPLE_COUNT_1_BIT ? &colorAttachmentResolveRef : nullptr;

	VkSubpassDependency dependency = {};
	dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	dependency.dstSubpass = 0;
	dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependency.srcAccessMask = 0;
	dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependency.dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;


	std::vector<VkAttachmentDescription> attachments = { colorAttachment, depthAttachment };
	if (msaaSamples != VK_SAMPLE_COUNT_1_BIT)
		attachments.emplace_back(colorAttachmentResolve);

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	renderPassInfo.pAttachments = attachments.data();
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 1;
	renderPassInfo.pDependencies = &dependency;

	if (vkCreateRenderPass(device, &renderPassInfo, allocator, &renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");
}

void OutputWindow::_CreateDeferredRenderPass(VkFormat format, VkSampleCountFlagBits samples)
{
	// create render passes
	VkExtent2D winExtent = _GetVkExtentSize();
	deferredFrameBuf.width = winExtent.width;
	deferredFrameBuf.height = winExtent.height;

	samples = VK_SAMPLE_COUNT_1_BIT;

	_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, winExtent, deferredFrameBuf.albedo);
	_CreateAttachment(VK_FORMAT_R16G16_SNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, winExtent, deferredFrameBuf.normals);
	_CreateAttachment(VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, winExtent, deferredFrameBuf.pbr);
	_CreateAttachment(_FindDepthFormat(), VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, winExtent, deferredFrameBuf.depth);

	std::vector<VkAttachmentDescription> attachmentDescs;
	std::vector<VkAttachmentReference> colorReferences;
	VkAttachmentReference depthReference;
	std::vector<VkImageView> attachments;

	deferredFrameBuf.ForEachFrameBuffer([&](FrameBufferAttachment &fba) {
		VkAttachmentDescription desc;
		desc.flags = 0;
		desc.samples = samples;
		desc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
		desc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
		desc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		desc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		desc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;		
		desc.format = fba.format;
		if (fba.usage & VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT)
		{
			desc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
			depthReference = { static_cast<uint32_t>(attachmentDescs.size()), VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL };
		}
		else {
			desc.finalLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
			colorReferences.push_back({ static_cast<uint32_t>(attachmentDescs.size()), VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL });
		}
		attachmentDescs.push_back(desc);
		attachments.push_back(fba.view);
		
	});

	VkSubpassDescription subpass = {};
	subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpass.pColorAttachments = colorReferences.data();
	subpass.colorAttachmentCount = static_cast<uint32_t>(colorReferences.size());
	subpass.pDepthStencilAttachment = &depthReference;
//	subpass.pResolveAttachments = samples != VK_SAMPLE_COUNT_1_BIT ? &colorAttachmentResolveRef : nullptr;

	std::array<VkSubpassDependency, 2> dependencies;

	dependencies[0].srcSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[0].dstSubpass = 0;
	dependencies[0].srcStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[0].dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[0].srcAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[0].dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[0].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	dependencies[1].srcSubpass = 0;
	dependencies[1].dstSubpass = VK_SUBPASS_EXTERNAL;
	dependencies[1].srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
	dependencies[1].dstStageMask = VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT;
	dependencies[1].srcAccessMask = VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;
	dependencies[1].dstAccessMask = VK_ACCESS_MEMORY_READ_BIT;
	dependencies[1].dependencyFlags = VK_DEPENDENCY_BY_REGION_BIT;

	VkRenderPassCreateInfo renderPassInfo = {};
	renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassInfo.pAttachments = attachmentDescs.data();
	renderPassInfo.attachmentCount = static_cast<uint32_t>(attachmentDescs.size());
	renderPassInfo.subpassCount = 1;
	renderPassInfo.pSubpasses = &subpass;
	renderPassInfo.dependencyCount = 2;
	renderPassInfo.pDependencies = dependencies.data();


	if (vkCreateRenderPass(device, &renderPassInfo, allocator, &deferredFrameBuf.renderPass) != VK_SUCCESS)
		throw std::runtime_error("failed to create render pass!");


	VkFramebufferCreateInfo fbufCreateInfo = {};
	fbufCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
	fbufCreateInfo.pNext = NULL;
	fbufCreateInfo.renderPass = deferredFrameBuf.renderPass;
	fbufCreateInfo.pAttachments = attachments.data();
	fbufCreateInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
	fbufCreateInfo.width = deferredFrameBuf.width;
	fbufCreateInfo.height = deferredFrameBuf.height;
	fbufCreateInfo.layers = 1;
	if (vkCreateFramebuffer(device, &fbufCreateInfo, nullptr, &deferredFrameBuf.frameBuffer) != VK_SUCCESS)
		throw std::runtime_error("failed to create frame buffer!");

	// sampler 
	VkSamplerCreateInfo sampler{};
	sampler.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	sampler.maxAnisotropy = 1.0f;
	sampler.magFilter = VK_FILTER_NEAREST;
	sampler.minFilter = VK_FILTER_NEAREST;
	sampler.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	sampler.addressModeU = VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
	sampler.addressModeV = sampler.addressModeU;
	sampler.addressModeW = sampler.addressModeU;
	sampler.mipLodBias = 0.0f;
	sampler.maxAnisotropy = 1.0f;
	sampler.minLod = 0.0f;
	sampler.maxLod = 1.0f;
	sampler.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	if (vkCreateSampler(device, &sampler, nullptr, &deferredFrameBuf.colorSampler) != VK_SUCCESS)
		throw std::runtime_error("failed to sampler!");

	deferredFrameBuf.descriptionImageInfo.push_back({ deferredFrameBuf.colorSampler, deferredFrameBuf.albedo.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	deferredFrameBuf.descriptionImageInfo.push_back({ deferredFrameBuf.colorSampler, deferredFrameBuf.normals.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	deferredFrameBuf.descriptionImageInfo.push_back({ deferredFrameBuf.colorSampler, deferredFrameBuf.pbr.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });
	deferredFrameBuf.descriptionImageInfo.push_back({ VK_NULL_HANDLE, deferredFrameBuf.depth.view, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL });

	VkSemaphoreCreateInfo semaphoreInfo = {};
	semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
	if (vkCreateSemaphore(device, &semaphoreInfo, allocator, &deferredFrameBuf.deferredSemaphore) != VK_SUCCESS ||
		vkCreateSemaphore(device, &semaphoreInfo, allocator, &deferredFrameBuf.resolvingSemaphore) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create semaphores!");
	}
}

void OutputWindow::_LoadShaderPrograms()
{
	_CreateDescriptorPool();                            // create descriptor pool for all used shader programs

	shaders.foreach([&](CShaderProgram *&s) 
	{
		auto outputNames = s->GetOutputNames();			// select renderPass base on names of output attachments in fragment shader
		if (outputNames.size() == 1 && Utils::icasecmp(outputNames[0], "outColor"))
		{
			s->SetRenderPassAndMsaaSamples(renderPass, msaaSamples);
			s->CreateGraphicsPipeline();
			s->CreateDescriptorSets();
			s->UpdateDescriptorSets();
		}
	});

}

void OutputWindow::_CleanupShaderPrograms()
{
	shaders.foreach([&](CShaderProgram *&s) {
		s->Release();
	});
	forwardRenderingShaders.clear();
}

void OutputWindow::UpdateUniformBuffer()
{
	if (!sharedUboData)
		return;
	static auto startTime = std::chrono::high_resolution_clock::now();

	auto currentTime = std::chrono::high_resolution_clock::now();
	float time = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - startTime).count() / 1000.0f;
	time = fmod(time, 2.0f);
	if (time > 1.0f)
		time = 2.0f - time;
	//	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
		//	ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 1.0f, 0.0f));

	//	UniformBufferObject ubo = {};
	auto &ubo = *sharedUboData;
	time = 0;
//	ubo.view = glm::lookAt(glm::vec3(2.0f, 2.0f + time, 2.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	ubo.view = glm::lookAt(glm::vec3(100.0f, 100.0f + time*50.0f, 100.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	
	TRACEM(ubo.view);
	ubo.proj = glm::perspective(glm::radians(45.0f), swapChainExtent.width / (float)swapChainExtent.height, 0.1f, 550.0f);
	ubo.proj[1][1] *= -1;
}

void OutputWindow::RecreateSwapChain()
{
	vkDeviceWaitIdle(device);

	_CleanupSwapChain();

	if (_CreateSwapChain())
	{
		_CreateCommandBuffers();
	}
}

void OutputWindow::_RecreateCommandBuffers()
{
	vkDeviceWaitIdle(device);

	vkFree(commandPool, commandBuffers);
	_CreateCommandBuffers();
	recreateCommandBuffers = false;
}

bool OutputWindow::_SimpleAcquireNextImage(uint32_t &imageIndex)
{
	if (!swapChain || resizeWindow) {
		RecreateSwapChain();
		return false;
	}

	if (recreateCommandBuffers)
	{
		_RecreateCommandBuffers();
	}


	VkResult result = vkAcquireNextImageKHR(device, swapChain, std::numeric_limits<uint64_t>::max(), imageAvailableSemaphore, VK_NULL_HANDLE, &imageIndex);
	if (result == VK_ERROR_OUT_OF_DATE_KHR)
	{
		RecreateSwapChain();
		return false;
	}
	else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR)
	{
		throw std::runtime_error("failed to acquire swap chain image!");
	}
	if (!sharedUboData)
	{
		return false;
	}

	return true;
}

void OutputWindow::_SimpleQueueSubmit(VkSemaphore &waitSemaphore, VkSemaphore &signalSemaphore, VkCommandBuffer &commandBuffer)
{

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

	VkSemaphore waitSemaphores[] = { waitSemaphore };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSemaphore signalSemaphores[] = { signalSemaphore };

	submitInfo.waitSemaphoreCount = 1;
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;

	submitInfo.signalSemaphoreCount = 1;
	submitInfo.pSignalSemaphores = signalSemaphores;

	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE) != VK_SUCCESS) {
		throw std::runtime_error("failed to submit draw command buffer!");
	}
}

void OutputWindow::_SimplePresent(VkSemaphore &waitSemaphore, uint32_t imageIndex)
{
	VkSemaphore waitSemaphores[] = { waitSemaphore };

	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

	presentInfo.waitSemaphoreCount = 1;
	presentInfo.pWaitSemaphores = waitSemaphores;

	VkSwapchainKHR swapChains[] = { swapChain };
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = swapChains;
	presentInfo.pImageIndices = &imageIndex;

	presentInfo.pResults = nullptr; // Optional

	VkResult result = vkQueuePresentKHR(presentQueue, &presentInfo);
	if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
		RecreateSwapChain();
	}
	else if (result != VK_SUCCESS) {
		throw std::runtime_error("failed to present swap chain image!");
	}
}

void OutputWindow::DrawFrame()
{
	uint32_t imageIndex;
	if (!_SimpleAcquireNextImage(imageIndex))
		return;
	bool deferredPresent = true;

	_SimpleQueueSubmit(imageAvailableSemaphore, deferredFrameBuf.deferredSemaphore, *commandBuffers.rbegin());
	_SimpleQueueSubmit(deferredFrameBuf.deferredSemaphore, renderFinishedSemaphore, commandBuffers[imageIndex]);
	_SimplePresent(renderFinishedSemaphore, imageIndex);

	vkQueueWaitIdle(presentQueue);
}

CShaderProgram * OutputWindow::AddShader(const char * shader)
{
	auto sh = shaders.find(shader);
	if (sh)
		return sh; // can't add second shader program with same name....

	sh = shaders.add(shader, this);
	sh->LoadProgram(shader); 

	auto outputNames = sh->GetOutputNames();			// select renderPass base on names of output attachments in fragment shader
	if (outputNames.size() == 1 && Utils::icasecmp(outputNames[0], "outColor"))
	{
		forwardRenderingShaders.push_back(sh);
		sh->SetRenderPassAndMsaaSamples(renderPass, msaaSamples);
	}
	else 
	{ // deferred
		deferredFrameBuf.shaders.push_back(sh);
		sh->SetRenderPassAndMsaaSamples(deferredFrameBuf.renderPass, msaaSamples);
	}
	
	if (renderPass != nullptr)
	{
		// prepare shader to be used.
		sh->CreateGraphicsPipeline();
		sh->CreateDescriptorSets();
		sh->UpdateDescriptorSets();
	}
	return sh;
}

CShaderProgram * OutputWindow::ReloadShader(const char * shader)
{
	auto sh = shaders.find(shader);
	if (!sh)
		return nullptr;

	sh->LoadProgram(shader);
	if (renderPass != nullptr)
	{
		// prepare shader to be used.
//		sh->SetRenderPassAndMsaaSamples(renderPass, msaaSamples);
		sh->CreateGraphicsPipeline();
//		sh->CreateDescriptorSets();
//		sh->UpdateDescriptorSets();
		
		BufferRecreationNeeded();
	}
	return sh;
}

void OutputWindow::GetShaderParams(const char *shader, BAMS::CORE::Properties **props)
{
	auto sh = shaders.find(shader);
	if (!sh) return;
	*props = sh->GetProperties();
}


void OutputWindow::GetObjectParams(const char *objectName, BAMS::CORE::Properties **props)
{
	auto oi = objects.find(objectName);
	if (!oi) return;
	*props = oi->GetProperties();
}

void OutputWindow::UpdateDrawCommands()
{
	BufferRecreationNeeded();
}

VkDescriptorImageInfo *OutputWindow::GetDescriptionImageInfo(const char * attachmentName)
{
	if (strcmp(attachmentName, "samplerAlbedo") == 0)
	{
		return &deferredFrameBuf.descriptionImageInfo[0];
	}
	else if (strcmp(attachmentName, "samplerNormal") == 0)
	{
		return &deferredFrameBuf.descriptionImageInfo[1];
	}
	else if (strcmp(attachmentName, "samplerPbr") == 0)
	{
		return &deferredFrameBuf.descriptionImageInfo[2];
	}
	else if (strcmp(attachmentName, "samplerDepth") == 0)
	{
		return &deferredFrameBuf.descriptionImageInfo[3];
	}
	return nullptr;
}

CShaderProgram * OutputWindow::_GetShader(const char *shader)
{
	return AddShader(shader);
}

ObjectInfo * OutputWindow::_GetObject(const char * name)
{
	return objects.find(name);
}

uint32_t OutputWindow::AddMesh(const char * mesh, const char * shader)
{
	auto sh = _GetShader(shader);
	if (!sh)
		return -1;

	return sh->AddMesh(mesh);
}

ObjectInfo * OutputWindow::AddObject(const char * name, const char * mesh, const char * shader)
{
	auto sh = _GetShader(shader);
	if (!sh)
		return nullptr;

	auto mid = sh->AddMesh(mesh);
	if (mid == -1)
		return nullptr;

	auto oid = sh->AddObject(mid);
	auto ret = objects.add(name, sh, mid, oid);
	ret->oid = oid;
	BufferRecreationNeeded();
	return ret;
}


bool OutputWindow::IsBufferFeatureSupported(VkFormat format, VkFormatFeatureFlagBits features)
{
	static VkFormatProperties pr;
	vkGetPhysicalDeviceFormatProperties(physicalDevice, format, &pr);
	return pr.bufferFeatures & features;
}
