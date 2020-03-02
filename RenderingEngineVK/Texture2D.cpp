#include "stdafx.h"


void CTexture2d::_LoadTexture(Image *pImg)
{
	VkTools vkt(vk);

	vkt.CreateImage(pImg, image, memory, view, format, mipLevels);

	vkt.CopyToStagingBuffer(pImg);

	vkt.TransitionImageLayout(image, 0, pImg->mipmaps, 0, pImg->faces*pImg->layers,
		0, VK_ACCESS_TRANSFER_WRITE_BIT,
		VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
		VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

	vkt.CopyBufferToImage(image, pImg);
	vkt.Execute();

	if (mipLevels != pImg->mipmaps)
	{
		vkt.TransitionImageLayout(image, 0, pImg->mipmaps, 0, pImg->faces*pImg->layers,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

		for (uint32_t mip = pImg->mipmaps; mip < mipLevels; mip++)
		{
			vkt.TransitionImageLayout(image, mip, 1, 0, pImg->faces*pImg->layers,
				0, VK_ACCESS_TRANSFER_WRITE_BIT,
				VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);

			vkt.BlitImage(image, mip - 1, mip, pImg);

			vkt.TransitionImageLayout(image, mip, 1, 0, pImg->faces*pImg->layers,
				VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_TRANSFER_READ_BIT,
				VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT);
		}
		vkt.TransitionImageLayout(image, 0, mipLevels, 0, pImg->faces*pImg->layers,
			VK_ACCESS_TRANSFER_READ_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	}
	else {
		vkt.TransitionImageLayout(image, 0, mipLevels, 0, pImg->faces*pImg->layers,
			VK_ACCESS_TRANSFER_WRITE_BIT, VK_ACCESS_SHADER_READ_BIT,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT);

	}
	vkt.Execute();

	// === create sampler ============================================================================================================

	sampler = vkt.CreateSampler(mipLevels, true);

	descriptor.sampler = sampler;
	descriptor.imageView = view;
	descriptor.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;

	vk->_MarkDescriptorSetsInvalid(&descriptor);
	TRACE("Created texture: " << resImg.GetName() << "\n");
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

