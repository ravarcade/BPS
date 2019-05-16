#pragma once

class Texture2D
{
public:
	Texture2D();
	~Texture2D();

protected:
	VkSampler sampler = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format;
	VkDescriptorImageInfo descriptor;
	VkImageLayout imageLayout;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mipLevels;

	friend class OutputWindow;
};
