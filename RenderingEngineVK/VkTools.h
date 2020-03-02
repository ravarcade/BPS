#pragma once

class VkTools
{
private:
	OutputWindow *vk;
	VkBuffer stagingBuffer;
	VkDeviceMemory stagingBufferMemory;
	VkCommandBuffer commandBuffer;

	void _CommandBufferRequired()
	{
		if (commandBuffer == VK_NULL_HANDLE)
		{
			commandBuffer = vk->_BeginSingleTimeCommands();
		}
	}

public:
	VkTools(OutputWindow *_vk) :
		vk(_vk),
		stagingBuffer(VK_NULL_HANDLE),
		stagingBufferMemory(nullptr),
		commandBuffer(VK_NULL_HANDLE)
	{}

	~VkTools()
	{
		if (commandBuffer) {
			vkFreeCommandBuffers(vk->device, vk->commandPool, 1, &commandBuffer);
		}

		vk->vkDestroy(stagingBuffer);
		vk->vkFree(stagingBufferMemory);
	}

	void CopyToStagingBuffer(Image *pImg, uint32_t mip = -1)
	{
		if (mip == -1) 
		{
			CopyToStagingBuffer(pImg->rawImage,
				pImg->GetImageOffset(pImg->mipmaps, 0, 0));
		}
		else {
			CopyToStagingBuffer(pImg->rawImage + pImg->GetImageOffset(mip, 0, 0),
				pImg->GetImageOffset(mip + 1, 0, 0) - pImg->GetImageOffset(mip, 0, 0));
		}
	}

	void CopyToStagingBuffer(void *data, size_t size)
	{
		vk->_CreateBuffer(
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
			stagingBuffer, stagingBufferMemory);

		void *pDevMem = nullptr;
		vkMapMemory(vk->device, stagingBufferMemory, 0, size, 0, &pDevMem);
		memcpy(pDevMem, data, size);
		vkUnmapMemory(vk->device, stagingBufferMemory);
	}

	VkPipelineStageFlags sourceStage;

	void TransitionImageLayout(VkImage image, uint32_t baseMipmap, uint32_t levelCount,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
	{
		TransitionImageLayout(image, baseMipmap, levelCount, 0, 1, srcAccessMask, dstAccessMask, oldLayout, newLayout, srcStageMask, dstStageMask);
	}

	void TransitionImageLayout(VkImage image, uint32_t baseMipmap, uint32_t mipmapsCount, uint32_t baseLayer, uint32_t layersCount,
		VkAccessFlags srcAccessMask,
		VkAccessFlags dstAccessMask,
		VkImageLayout oldLayout, VkImageLayout newLayout,
		VkPipelineStageFlags srcStageMask, VkPipelineStageFlags dstStageMask)
	{
		_CommandBufferRequired();

		VkImageMemoryBarrier barrier = {};
		barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
		barrier.image = image;
		barrier.srcAccessMask = srcAccessMask;
		barrier.dstAccessMask = dstAccessMask;
		barrier.oldLayout = oldLayout;
		barrier.newLayout = newLayout;
		barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
		
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		barrier.subresourceRange.baseMipLevel = baseMipmap;
		barrier.subresourceRange.levelCount = mipmapsCount;
		barrier.subresourceRange.baseArrayLayer = baseLayer;
		barrier.subresourceRange.layerCount = layersCount;

		vkCmdPipelineBarrier(
			commandBuffer,
			srcStageMask,
			dstStageMask,
			0,
			0, nullptr,
			0, nullptr,
			1, &barrier);
	}
	void CopyBufferToImage(VkImage image, Image *pImg)
	{
		_CommandBufferRequired();

		VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

		// region of dstImage to update
		std::vector<VkBufferImageCopy> bufferCopyRegions;

		for (uint32_t mip = 0; mip < pImg->mipmaps; ++mip)
		{
			for (uint32_t layer = 0; layer < pImg->layers; ++layer)
			{
				for (uint32_t face = 0; face < pImg->faces; ++face)
				{
					auto offset = pImg->GetImageOffset(mip, layer, face);
					VkBufferImageCopy bufferCopyRegion = {};
					bufferCopyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
					bufferCopyRegion.imageSubresource.mipLevel = mip;
					bufferCopyRegion.imageSubresource.baseArrayLayer = face | layer; // we do "OR" because face = 0 or layer = 0
					bufferCopyRegion.imageSubresource.layerCount = 1;
					bufferCopyRegion.imageExtent.width = std::max(pImg->width >> mip, (uint32_t)1);
					bufferCopyRegion.imageExtent.height = std::max(pImg->height >> mip, (uint32_t)1);
					bufferCopyRegion.imageExtent.depth = 1;
					bufferCopyRegion.bufferOffset = offset;
					bufferCopyRegions.push_back(bufferCopyRegion);
				}
			}
		}

		vkCmdCopyBufferToImage(
			commandBuffer,
			stagingBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			static_cast<uint32_t>(bufferCopyRegions.size()),
			bufferCopyRegions.data());
	}

	void BlitImage(VkImage image, uint32_t srcMip, uint32_t dstMip, Image *pImg)
	{
		_CommandBufferRequired();

		VkImageBlit imageBlit{}; // note {} clears stucture!
		// Source
		imageBlit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageBlit.srcSubresource.mipLevel = srcMip;
		imageBlit.srcSubresource.baseArrayLayer = 0;
		imageBlit.srcSubresource.layerCount = pImg->faces*pImg->layers;
		imageBlit.srcOffsets[1].x = std::max(int32_t(pImg->width >> srcMip), 1);
		imageBlit.srcOffsets[1].y = std::max(int32_t(pImg->height >> srcMip), 1);
		imageBlit.srcOffsets[1].z = 1;

		// Destination
		imageBlit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;		
		imageBlit.dstSubresource.mipLevel = dstMip;
		imageBlit.dstSubresource.baseArrayLayer = 0;
		imageBlit.dstSubresource.layerCount = pImg->faces*pImg->layers;
		imageBlit.dstOffsets[1].x = std::max(int32_t(pImg->width >> dstMip), 1);
		imageBlit.dstOffsets[1].y = std::max(int32_t(pImg->height >> dstMip), 1);
		imageBlit.dstOffsets[1].z = 1;


		vkCmdBlitImage(
			commandBuffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&imageBlit,
			VK_FILTER_LINEAR);
	}

	void Execute()
	{
		if (commandBuffer) {
			vk->_EndSingleTimeCommands(commandBuffer);
			commandBuffer = VK_NULL_HANDLE;
		}
	}

	void CreateImage(
		Image *pImg,
		VkImage& outImage, VkDeviceMemory& outImageMemory, VkImageView &outView,
		VkFormat& outFormat, uint32_t &outMipmaps)
	{
		outFormat = VK_FORMAT_R8G8B8A8_UNORM;
		switch (pImg->format) {
		case PF_R8G8B8_UNORM:  outFormat = VK_FORMAT_R8G8B8_UNORM; break;
		case PF_BC2_UNORM:  outFormat = VK_FORMAT_BC2_UNORM_BLOCK; break;
			// more formats? add here
		}

		outMipmaps = vk->_mipLevels(pImg->width, pImg->height);
		bool isBuildMipmapsRequired = pImg->mipmaps != outMipmaps;

		VkFormatProperties formatProperties;
		vkGetPhysicalDeviceFormatProperties(vk->physicalDevice, outFormat, &formatProperties);
		bool isBlitSupported =
			(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_SRC_BIT) &&
			(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_BLIT_DST_BIT);

		if (!isBlitSupported && isBuildMipmapsRequired) { // fallback to "no mipmaps"... 
			isBuildMipmapsRequired = false;
			outMipmaps = pImg->mipmaps;
		}

		vk->_CreateImage(outImage, outImageMemory, pImg->width, pImg->height, outMipmaps, 
			pImg->faces * pImg->layers,
			VK_SAMPLE_COUNT_1_BIT,
			outFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | (isBuildMipmapsRequired ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0),
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
			(pImg->isCube ? VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT : 0) | (pImg->isArray ? 0 : 0));

		outView = vk->_CreateImageView(outImage, outFormat, VK_IMAGE_ASPECT_COLOR_BIT, pImg->isCube ? VK_IMAGE_VIEW_TYPE_CUBE : VK_IMAGE_VIEW_TYPE_2D);
	}

	VkSampler CreateSampler(uint32_t mipmaps, bool enableAnisotrophy)
	{
		VkSamplerCreateInfo samplerInfo = {};
		samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
		samplerInfo.magFilter = VK_FILTER_LINEAR;
		samplerInfo.minFilter = VK_FILTER_LINEAR;
		samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
		samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
		samplerInfo.mipLodBias = 0.f;
		samplerInfo.compareOp = VK_COMPARE_OP_NEVER;
		samplerInfo.minLod = 0.f;
		samplerInfo.maxLod = static_cast<float>(mipmaps);
		samplerInfo.maxAnisotropy = 1.0;
		samplerInfo.anisotropyEnable = VK_FALSE;
		if (vk->devFeatures.samplerAnisotropy && enableAnisotrophy)
		{
			samplerInfo.maxAnisotropy = vk->devProperties.limits.maxSamplerAnisotropy;
			samplerInfo.anisotropyEnable = VK_TRUE;
		}
		samplerInfo.borderColor = VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
		VkSampler sampler;
		vkCreateSampler(vk->device, &samplerInfo, vk->allocator, &sampler);
		return sampler;
	}
};
