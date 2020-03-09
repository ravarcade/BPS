#pragma once

class OutputWindow;


class CTexture2d
{
public:
	CTexture2d(OutputWindow *outputWindow) : vk(outputWindow) {};
	~CTexture2d() {};
	void Release() {};


	void LoadResTexture(IResource * textureResource);

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

	BAMS::CResImage resImg;
	void _LoadTexture(Image *img);

	friend class OutputWindow;
	friend class VkImGui;
	OutputWindow *vk;	
};
