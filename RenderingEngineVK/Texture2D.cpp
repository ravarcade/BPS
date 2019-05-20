#include "stdafx.h"

CTexture2d::CTexture2d(OutputWindow *outputWindow) 
	: vk(outputWindow)
{
}

CTexture2d::~CTexture2d()
{
}

void CTexture2d::LoadTexture(const char * textureResourceName)
{
	CResourceManager rm;
	auto res = rm.Get<CResImage>(textureResourceName);
	auto img = res.GetImage(true);
	uint32_t mipmapLevels = 0;
	VkFormat format = VK_FORMAT_R8G8B8A8_UNORM;

	// create texture
	vk->_CreateImage(img->width, img->height, 1, VK_SAMPLE_COUNT_1_BIT, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_TILING_OPTIMAL, VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, image, memory);
	vk->_TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

	// copy texture from ram to gpu
	vk->CopyImage(image, img);
	vk->_TransitionImageLayout(image, VK_FORMAT_R8G8B8A8_UNORM, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	// create sampler
	VkSamplerCreateInfo samplerInfo = {};
	samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerInfo.magFilter = VK_FILTER_LINEAR;
	samplerInfo.minFilter = VK_FILTER_LINEAR;
	samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerInfo.mipLodBias = 0.0f;
	samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
	samplerInfo.minLod = 0.0f;

	// Set max level-of-detail to mip level count of the texture
	samplerInfo.maxLod = static_cast<float>(mipmapLevels);

	// TODO: set anisotropy
	samplerInfo.maxAnisotropy = 1.0;
	samplerInfo.anisotropyEnable = VK_FALSE;
	samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
	vkCreateSampler(vk->device, &samplerInfo, vk->allocator, &sampler);

	// view
	view = vk->_CreateImageView(image, format, VK_IMAGE_ASPECT_COLOR_BIT);

	imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = imageLayout;
}

void CTexture2d::Release()
{
}
