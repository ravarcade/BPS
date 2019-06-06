#include "stdafx.h"

void CDescriptorSetsMananger::Prepare(OutputWindow *ow)
{
	vk = ow;

	// check default values and compare with limits:
	VkPhysicalDeviceProperties physicalDeviceProperties;
	vkGetPhysicalDeviceProperties(vk->physicalDevice, &physicalDeviceProperties);

	auto &SetLimit = [&](uint32_t maxLimit, std::initializer_list<VkDescriptorType> desc)
	{
		maxLimit = maxLimit > 10 ? maxLimit / 5 : maxLimit / 2; // we use max 20% of available resources... never ask for more.
		for (auto i : desc)
		{
			if (default_DescriptorSizes[i] > maxLimit)
				default_DescriptorSizes[i] = maxLimit;
		}
	};

	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetSamplers, { VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetSampledImages, { VK_DESCRIPTOR_TYPE_SAMPLER, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetStorageImages, { VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetUniformBuffers, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetUniformBuffersDynamic, { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetStorageBuffers, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetStorageBuffersDynamic, { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC });
	SetLimit(physicalDeviceProperties.limits.maxDescriptorSetInputAttachments, { VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT });

	_CreateNewDescriptorPool();
}

void CDescriptorSetsMananger::Clear()
{
	vk->vkDestroy(descriptorPool);
	for (auto &dp : oldDescriptorPools)
		vk->vkDestroy(dp);
	oldDescriptorPools.clear();
	availableDescriptorSets = 0;
}

VkDescriptorSet CDescriptorSetsMananger::CreateDescriptorSets(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, std::vector<VkDescriptorPoolSize> &descriptorRequirments)
{
	// check if we need new pool
	bool needNewPool = descriptorPool == VK_NULL_HANDLE || !availableDescriptorSets;
	for (uint32_t i = 0; i < descriptorRequirments.size(); ++i)
	{
		if (needNewPool || availableDescriptorTypes[descriptorRequirments[i].type].descriptorCount < descriptorRequirments[i].descriptorCount)
		{
			_CreateNewDescriptorPool();
			break;
		}
	}

	--availableDescriptorSets;
	++stats_usedDescriptorSets;
	for (auto &req : descriptorRequirments)
	{
		availableDescriptorTypes[req.type].descriptorCount -= req.descriptorCount;
		stats_usedDescriptorTypes[req.type] += req.descriptorCount;
	}

	VkDescriptorSet descriptorSet = nullptr;
	VkDescriptorSetAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	allocInfo.descriptorPool = descriptorPool;
	allocInfo.descriptorSetCount = static_cast<uint32_t>(descriptorSetLayouts.size());
	allocInfo.pSetLayouts = descriptorSetLayouts.data();

	if (vkAllocateDescriptorSets(vk->device, &allocInfo, &descriptorSet) != VK_SUCCESS) {
		throw std::runtime_error("failed to allocate descriptor set!");
	}

	return descriptorSet;
}

std::vector<VkDescriptorSetLayout> CDescriptorSetsMananger::CreateDescriptorSetLayouts(CShadersReflections::ResourceLayout & layout)
{
	std::vector<VkDescriptorSetLayout> dsl;

	// find last set
	uint32_t lastSet = 0, i = 0;
	for (auto &set : layout.descriptorSets)
	{
		if (set.uniform_buffer_mask ||
			set.sampled_image_mask)
			lastSet = i;
		++i;
	}

	// create all needed DescriptorSetsLayouts and add it to dsl
	for (uint32_t setNo = 0; setNo <= lastSet; ++setNo)
	{
		auto &set = layout.descriptorSets[setNo];
		std::vector<VkDescriptorSetLayoutBinding> bindings;

		if (set.uniform_buffer_mask)
		{
			for (unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
			{
				uint32_t bitMask = 1u << i;
				uint32_t descriptorCount = set.descriptorCount[i];
				uint32_t stages = set.stages[i];

				if (set.uniform_buffer_mask & bitMask) {
					bindings.push_back({ i, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, descriptorCount, stages, nullptr });
				}
			}
		}

		if (set.sampled_image_mask)
		{
			for (unsigned i = 0; i < VULKAN_NUM_BINDINGS; i++)
			{
				uint32_t bitMask = 1u << i;
				uint32_t descriptorCount = set.descriptorCount[i];
				uint32_t stages = set.stages[i];

				if (set.sampled_image_mask & bitMask) {
					bindings.push_back({ i, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, descriptorCount, stages, nullptr });
				}
			}

		}

		if (bindings.size())
		{
			VkDescriptorSetLayoutCreateInfo layoutInfo = { VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO };
			layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
			layoutInfo.pBindings = bindings.data();

			VkDescriptorSetLayout descriptorSetLayout;

			if (vkCreateDescriptorSetLayout(vk->device, &layoutInfo, vk->allocator, &descriptorSetLayout) != VK_SUCCESS)
				throw std::runtime_error("failed to create descriptor set layout!");

			dsl.push_back(descriptorSetLayout);
		}
	}
	return std::move(dsl);
}

std::vector<VkDescriptorPoolSize> CDescriptorSetsMananger::CreateDescriptorRequirments(CShadersReflections::ResourceLayout & layout)
{
	std::vector<VkDescriptorPoolSize> req;

	//
#define SETDESCRIPTORSREQUIRMENTS(t,x) \
	{ \
		uint32_t num = 0; \
		for (auto &set : layout.descriptorSets) \
		{ \
			if (set.##x) \
				num += Utils::count_bits(set.##x); \
		} \
		if (num) \
			req.push_back({t,num}); \
	}

	SETDESCRIPTORSREQUIRMENTS(VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, uniform_buffer_mask);
	SETDESCRIPTORSREQUIRMENTS(VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, sampled_image_mask);
	// add other descriptor types

	return std::move(req);
}

void CDescriptorSetsMananger::_CreateNewDescriptorPool()
{
	if (descriptorPool != VK_NULL_HANDLE) {
		oldDescriptorPools.push_back(descriptorPool);
		descriptorPool = VK_NULL_HANDLE;
	}

	availableDescriptorSets = default_AvailableDesciprotrSets;
	for (uint32_t i = 0; i < VK_DESCRIPTOR_TYPE_RANGE_SIZE; ++i)
	{
		availableDescriptorTypes[i].type = static_cast<VkDescriptorType>(VK_DESCRIPTOR_TYPE_BEGIN_RANGE + i);
		availableDescriptorTypes[i].descriptorCount = default_DescriptorSizes[i];
	}

	//		_AddOldLimits(shaders, count);

	VkDescriptorPoolCreateInfo poolInfo = {};
	poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolInfo.poolSizeCount = static_cast<uint32_t>(COUNT_OF(availableDescriptorTypes));
	poolInfo.pPoolSizes = availableDescriptorTypes;
	poolInfo.maxSets = availableDescriptorSets;

	if (vkCreateDescriptorPool(vk->device, &poolInfo, vk->allocator, &descriptorPool) != VK_SUCCESS) {
		throw std::runtime_error("failed to create descriptor pool!");
	}
}

uint32_t CDescriptorSetsMananger::default_AvailableDesciprotrSets = 100;
uint32_t CDescriptorSetsMananger::default_DescriptorSizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = {
	0,  // VK_DESCRIPTOR_TYPE_SAMPLER = 0,
	20, // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER = 1,
	0,  // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE = 2,
	0,  // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE = 3,
	0,  // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER = 4,
	0,  // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER = 5,
	10, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER = 6,
	0,  // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER = 7,
	0,  // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC = 8,
	0,  // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC = 9,
	0,  // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT = 10, 
};
uint32_t CDescriptorSetsMananger::stats_usedDescriptorSets = 0;
uint32_t CDescriptorSetsMananger::stats_usedDescriptorTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE] = { 0 };
