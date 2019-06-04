
class OutputWindow;

enum EDrawOrder {
	DEFERRED = 0,
	FORWARD,
	MAX_DRAWORDER
};

struct ObjectInfo {
	CShaderProgram *shader;
	uint32_t oid;
	ObjectInfo(CShaderProgram *_shader, uint32_t _oid) : shader(_shader), oid(_oid) {}
	Properties *GetProperties() { return shader->GetProperties(oid); }
};

class OutputWindow
{
private:
	struct UpdateInfo {
		bool updateRequired;
		VkBuffer UBO = VK_NULL_HANDLE;
		
	};
	struct UpdateFlags {
		bool commandBuffers[MAX_DRAWORDER];
//		std::vector<
	};
	UpdateFlags updateFlags;

	struct SharedUniformBufferObject
	{
		//glm::mat4 model; // is push constant now
		glm::mat4 view;
		glm::mat4 proj;
	};

	struct FrameBufferAttachment 
	{
		VkImage image;
		VkDeviceMemory memory;
		VkImageView view;
		VkFormat format;
		VkImageUsageFlags usage;
	};

	struct DeferredFrameBuffers
	{
		DeferredFrameBuffers() {
			memset(this, 0, sizeof(DeferredFrameBuffers));
		}

		uint32_t width, height;
		FrameBufferAttachment frameBufferAttachments[3];
		FrameBufferAttachment depth;
		VkFramebuffer frameBuffer;

		VkImageView depthView; // // we have 2 views of depth attachment, one is created to use in in depth reconstruction (read only) and one if for normal Z-Buffer write/tests

		VkRenderPass renderPass;

		VkSampler colorSampler;
		VkSemaphore deferredSemaphore;
		VkSemaphore resolvingSemaphore;
		std::vector<CShaderProgram*> shaders;
		CShaderProgram *resolveShader = nullptr;

		template<typename T>
		void ForEachFrameBuffer(T f) 
		{
			for (auto &fba : frameBufferAttachments)
			{
				f(fba);
			}
			f(depth);
		};
		std::vector<VkDescriptorImageInfo>descriptionImageInfo;
	};

	VkInstance instance;				// required to: (1) create and destroy VkSurfaceKHR, (2) select physical device matching surface
	VkPhysicalDevice physicalDevice;
	GLFWwindow* window;
	const VkAllocationCallbacks* allocator;

	// ---- 
	VkSurfaceKHR surface;
	VkDevice device; // logical device
	VkQueue graphicsQueue;
	VkQueue presentQueue;
	VkQueue transferQueue;
	VkSwapchainKHR swapChain;
	std::vector<VkImage> swapChainImages;
	std::vector<VkImageView> swapChainImageViews;
	std::vector<VkFramebuffer> swapChainFramebuffers;
	VkFormat swapChainImageFormat;
	VkExtent2D swapChainExtent;

	DeferredFrameBuffers deferredFrameBuf;
	DeferredFrameBuffers forwardFrameBuf;

	VkCommandPool commandPool;
	VkCommandPool transferPool;
	VkSemaphore imageAvailableSemaphore;
	VkSemaphore renderFinishedSemaphore;
	VkSampleCountFlagBits msaaSamples = VK_SAMPLE_COUNT_1_BIT;
	VkBuffer        sharedUniformBuffer = nullptr;
	VkDeviceMemory  sharedUniformBufferMemory = nullptr;
	//		CShadersReflections cubeShader;
	VkViewport viewport;
	VkRect2D scissor;

	CDescriptorSetsMananger descriptorSetsManager;

	VkDescriptorSet currentDescriptorSet;

	std::vector<VkCommandBuffer> commandBuffers;
	bool resizeWindow = false;

	bool _IsDeviceSuitable(VkPhysicalDevice device);
	VkFormat _FindDepthFormat();
	VkFormat _FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// image functions
	void _CreateImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage & image, VkDeviceMemory & imageMemory);
	VkImageView _CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags);
	void _CreateAttachment(VkExtent2D extent, FrameBufferAttachment & attachment);
	void _CreateAttachment(VkFormat format, VkImageUsageFlags usage, VkExtent2D extent, FrameBufferAttachment &attachment);

	void _TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	uint32_t _FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkCommandBuffer _BeginSingleTimeCommands();
	void _EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void _CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	void _CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void * srcData, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
	void _CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);
	VkShaderModule _CreateShaderModule(const char * shaderName);

	VkExtent2D _GetVkExtentSize();

	bool _CreateSwapChain();
	void _CleanupSwapChain();

	void _CreateSharedUniform();
	void _CleanupSharedUniform();

	void _CreateRenderPass();
	void _CleanupRenderPass();
	
	void _CreateDeferredCommandBuffer(VkCommandBuffer & cb);
	void _CreateForwardCommandBuffer(VkCommandBuffer & cb, VkFramebuffer & frameBuffer);
	
	void _CreateCommandBuffers();
	void _RecreateCommandBuffers();
	void _CreateSimpleRenderPass(VkFormat format, VkSampleCountFlagBits samples);
	
	void _CleanupDeferredFramebuffer();
	void _CreateDeferredFramebuffer();
	void _CreateDeferredRenderPass(VkFormat format, VkSampleCountFlagBits samples);

	SharedUniformBufferObject *sharedUboData = nullptr;

	void _LoadShaderPrograms();
	void _CleanupTextures();
	void _CleanupShaderPrograms();
	void _Cleanup();

	CShaderProgram *_GetShader(const char *shader);
	ObjectInfo *_GetObject(const char *name);

	bool _SimpleAcquireNextImage(uint32_t & imageIndex);
	void _SimpleQueueSubmit(VkSemaphore & waitSemaphore, VkSemaphore & signalSemaphore, VkCommandBuffer & commandBuffer);
	void _SimplePresent(VkSemaphore & waitSemaphore, uint32_t imageIndex);

	bool _UpdateBeforeDrawFrame();
	void _RecreateSwapChain();
	void _CopyBufferToImage(VkImage dstImage, VkBuffer srcBuffer, VkBufferImageCopy & region, VkImageSubresourceRange & subresourceRange);

	static void _OnWindowSize(GLFWwindow *wnd, int width, int height);
public:
	OutputWindow();
	~OutputWindow() { _Cleanup(); }
	void Init();
	void Prepare(VkInstance _instance, GLFWwindow* _window, const VkAllocationCallbacks* _allocator);
	
	CDescriptorSetsMananger *GetDescriptorSetsManager() { return &descriptorSetsManager; }

	void Close(GLFWwindow *wnd = nullptr);

	void UpdateUniformBuffer();

	void DrawFrame();
	void PrepareShader(CShaderProgram * sh);
	bool Exist() { return device != VK_NULL_HANDLE; }
	void BufferRecreationNeeded() { updateFlags.commandBuffers[FORWARD] = true; updateFlags.commandBuffers[DEFERRED] = true; }
	bool IsValid() { return window != nullptr; }
	bool IsBufferFeatureSupported(VkFormat format, VkFormatFeatureFlagBits features);

	void CopyBuffer(VkBuffer dstBuf, VkDeviceSize offset, VkDeviceSize size, void *srcData);
	void CopyImage(VkImage dstImage, Image * srcImage);

	template<typename T> void vkDestroy(T &v) { if (v) { vkDestroyDevice(v, allocator); v = VK_NULL_HANDLE; } }

#ifdef Define_vkDestroy
#undef Define_vkDestroy
#endif
#define Define_vkDestroy(t) template<> void vkDestroy(Vk##t &v) { if (v) { vkDestroy##t(device, v, allocator); v = VK_NULL_HANDLE; } }
	Define_vkDestroy(Pipeline);
	Define_vkDestroy(PipelineLayout);
	Define_vkDestroy(ImageView);
	Define_vkDestroy(Image);
	Define_vkDestroy(Framebuffer);
	Define_vkDestroy(RenderPass);
	Define_vkDestroy(SwapchainKHR);
	Define_vkDestroy(DescriptorPool);
	Define_vkDestroy(DescriptorSetLayout);
	Define_vkDestroy(Buffer);
	Define_vkDestroy(Semaphore);
	Define_vkDestroy(CommandPool);
	Define_vkDestroy(ShaderModule);
	Define_vkDestroy(Sampler);
	template<> void vkDestroy(VkSurfaceKHR &v) { if (v) { vkDestroySurfaceKHR(instance, v, allocator); v = VK_NULL_HANDLE; } }
	template<> void vkDestroy(FrameBufferAttachment &v) { vkDestroy(v.image); vkDestroy(v.view); vkFree(v.memory); }

	void vkFree(VkDeviceMemory &v) { if (v) { vkFreeMemory(device, v, allocator); v = nullptr; } }
	void vkFree(VkCommandPool &v1, std::vector<VkCommandBuffer> &v2) { if (v1 && v2.size()) { vkFreeCommandBuffers(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }
	void vkFree(VkDescriptorPool &v1, std::vector<VkDescriptorSet> &v2) { if (v1 && v2.size()) { vkFreeDescriptorSets(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }

	std::vector<CShaderProgram*> forwardRenderingShaders;
	CStringHastable<CShaderProgram> shaders;
	CStringHastable<ObjectInfo> objects;

	ObjectInfo * AddObject(const char * name, const char * mesh, const char * shader);
	CShaderProgram *AddShader(const char * shader);
	CShaderProgram *ReloadShader(const char *shader);
	void GetShaderParams(const char * shader, Properties **params);
	void GetObjectParams(const char * objectName, uint32_t objectIdx, Properties ** props);
	void UpdateDrawCommands();

	void AddTexture(void *propVal, const char *textureName);

	// ------------------------ camera stuffs -------------------

	void SetCamera(const BAMS::PSET_CAMERA * cam);
	BAMS::PSET_CAMERA cameraSettings;

	VkDescriptorImageInfo *GetDescriptionImageInfo(const char *attachmentName);

	// ------------------------ for deferred rendering -----------

	template<> void vkDestroy(DeferredFrameBuffers &fb)
	{
		vkDestroy(fb.deferredSemaphore);
		vkDestroy(fb.resolvingSemaphore);
		vkDestroy(fb.colorSampler);
		vkDestroy(fb.frameBuffer);
		vkDestroy(fb.renderPass);
		for (auto &fba : fb.frameBufferAttachments)
			vkDestroy(fba);
		vkDestroy(fb.depthView);
		vkDestroy(fb.depth);
		fb.shaders.clear();
		fb.resolveShader = nullptr;
		fb.descriptionImageInfo.clear();
	}
	
	template<> void vkDestroy(UniformBuffer &ub)
	{
		if (ub.mappedBuffer)
		{
			vkUnmapMemory(device, ub.memory);
			ub.mappedBuffer = nullptr;
		}

		vkDestroy(ub.buffer);
		vkFree(ub.memory);
	}

	// ------------------------- for textures ----------------

	template<> void vkDestroy(CTexture2d &tex)
	{
		vkDestroy(tex.view);
		vkDestroy(tex.image);
		vkDestroy(tex.sampler);
		vkFree(tex.memory);
	}
	CStringHastable<CTexture2d> textures;

	friend CDescriptorSetsMananger;
	friend CShaderProgram;
	friend CTexture2d;
};
