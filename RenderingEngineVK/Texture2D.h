#pragma once

class CTexture2d
{
public:
	CTexture2d(OutputWindow *outputWindow);
	~CTexture2d();

	void LoadTexture(const char *textureResourceName);
	void Release();

	VkDescriptorImageInfo descriptor;
protected:
	VkSampler sampler = VK_NULL_HANDLE;
	VkImage image = VK_NULL_HANDLE;
	VkDeviceMemory memory = VK_NULL_HANDLE;
	VkImageView view = VK_NULL_HANDLE;
	VkFormat format;
	
	VkImageLayout imageLayout;
	uint32_t width;
	uint32_t height;
	uint32_t depth;
	uint32_t mipLevels;

	friend class OutputWindow;
	OutputWindow *vk;
};
