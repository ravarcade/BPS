#include "stdafx.h"

CTexture2d::CTexture2d(OutputWindow *outputWindow) 
	: vk(outputWindow)
{
}

CTexture2d::~CTexture2d()
{
}

void CTexture2d::_LoadTexture(Image *img)
{
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

	vk->_MarkDescriptorSetsInvalid(&descriptor);
	TRACE("Created texture: " << resImg.GetName());
}

void CTexture2d::LoadResTexture(const char * textureResourceName)
{
	CResourceManager rm;
	auto res = rm.FindExisting(textureResourceName, CResImage::GetType());
	if (!res)
	{

		TRACE("ERROR: Missing \"" << textureResourceName << "\" texture.\n");
		// assert(res != nullptr); // we don't have to crash here... if res == nullptr we will use defaultEmptyTexture
	}
	LoadResTexture(res);
}

void CTexture2d::LoadResTexture(IResource *textureResource)
{	
	if (textureResource)  // if resource exist, try to use it as texture
	{
		CResImage resImg(textureResource);
		auto img = resImg.GetImage();
		this->resImg = std::move(resImg);
		if (img->IsLoaded())
		{
			_LoadTexture(img);
			return;
		}
	}

	auto &pdt = vk->pDefaultEmptyTexture;
	if (!pdt) {
		uint8_t onePixel[4] = { 0xff, 0x7f, 0x7f, 0x7f };
		BAMS::Image defaultTextureImage(onePixel, 1, 1, PF_R8G8B8A8_UNORM);
		pdt = new CTexture2d(vk);
		pdt->_LoadTexture(&defaultTextureImage);
	}
	descriptor = pdt->descriptor;  // note: VkDescriptorImageInfo descriptor don't have any VK resources to destroy, so this plain copy is safe.
}

//void CTexture2d::UpdateTexture(Image *img)
//{
//	TRACE("Update Texture");
//}
//
//void CTexture2d::UpdateTexture(const char *textureResourceName)
//{
//	CResourceManager rm;
//	auto res = rm.FindExisting(textureResourceName, CResImage::GetType());
//	assert(res != nullptr);
//	if (!res)
//	{
//		TRACE("ERROR: Missing \"" << textureResourceName << "\" texture.\n");		
//	}
//	else {
//		UpdateTexture(res);
//	}
//}
//
//void CTexture2d::UpdateTexture(IResource * textureResource)
//{
//	CResImage resImg(textureResource);
//	auto img = resImg.GetImage(true);
//	assert(img != nullptr);
//	if (!img) {
//		TRACE("ERROR: Missing texture.\n");
//	}
//	else {
//		UpdateTexture(img);
//	}
//}

void CTexture2d::Release()
{
	
}
