class OutputWindow;

class CDescriptorSetsMananger {
public:
	CDescriptorSetsMananger() : vk(nullptr), descriptorPool(VK_NULL_HANDLE) {}
	~CDescriptorSetsMananger() { Clear(); }

	void Prepare(OutputWindow *ow);
	void Clear();
	VkDescriptorSet CreateDescriptorSets(std::vector<VkDescriptorSetLayout>& descriptorSetLayouts, std::vector<VkDescriptorPoolSize> &descriptorRequirments);
	std::vector<VkDescriptorSetLayout> CreateDescriptorSetLayouts(CShadersReflections::ResourceLayout &layout);
	std::vector<VkDescriptorPoolSize> CreateDescriptorRequirments(CShadersReflections::ResourceLayout &layout);

private:
	VkDescriptorPoolSize availableDescriptorTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
	uint32_t availableDescriptorSets;

	VkDescriptorPool descriptorPool;
	std::vector<VkDescriptorPool> oldDescriptorPools;

	OutputWindow *vk;

	void _CreateNewDescriptorPool();
	void _AddOldLimits(CShaderProgram **shaders, uint32_t count);

	// params:
	static uint32_t default_AvailableDesciprotrSets;
	static uint32_t default_DescriptorSizes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];


	// stats:
	static uint32_t stats_usedDescriptorSets;
	static uint32_t stats_usedDescriptorTypes[VK_DESCRIPTOR_TYPE_RANGE_SIZE];
};


