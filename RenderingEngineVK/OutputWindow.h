
class OutputWindow;
class VkImGui;
class VkTools;

enum EDrawOrder {
	DEFERRED = 0,
	DEFERRED_RESOLVE,
	FORWARD,
	MAX_DRAWORDER
};

struct ObjectInfo {
	CShaderProgram *shader;
	uint32_t oid;
	Properties *GetProperties() { return shader->GetProperties(oid); }
};

class OutputWindow : public iInputCallback
{
private:
	struct UpdateInfo {
		bool updateRequired;
		VkBuffer UBO = VK_NULL_HANDLE;
		
	};
	struct UpdateFlags {
		bool commandBuffers[MAX_DRAWORDER];
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
		std::vector<VkDescriptorImageInfo> descriptionImageInfo;
	};

	bool m_minimizeLag;
	uint32_t windowIdx;
	uint64_t frameCounter;
	VkInstance instance;				// required to: (1) create and destroy VkSurfaceKHR, (2) select physical device matching surface
	VkPhysicalDevice physicalDevice;
	GLFWwindow* window;
	const VkAllocationCallbacks* allocator;

	// ---- 
	VkSurfaceKHR surface;
	VkDevice device; // logical device
	VkPhysicalDeviceProperties devProperties;
	VkPhysicalDeviceFeatures devFeatures;
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


	CCamera camera;

	// ----------------------------------------

	bool _IsDeviceSuitable(VkPhysicalDevice device);
	VkFormat _FindDepthFormat();
	VkFormat _FindSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features);

	// image functions
	void _CreateImage(VkImage & outImage, VkDeviceMemory & outImageMemory, uint32_t width, uint32_t height, uint32_t mipLevels, uint32_t layers, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImageCreateFlags flags = 0);
	VkImageView _CreateImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, VkImageViewType viewType = VK_IMAGE_VIEW_TYPE_2D);
	void _CreateAttachment(VkExtent2D extent, FrameBufferAttachment & attachment);
	void _CreateAttachment(VkFormat format, VkImageUsageFlags usage, VkExtent2D extent, FrameBufferAttachment &attachment);

	void _TransitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout);

	uint32_t _FindMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties);
	VkCommandBuffer _BeginSingleTimeCommands();
	void _EndSingleTimeCommands(VkCommandBuffer commandBuffer);

	void _CreateBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer & buffer, VkDeviceMemory & bufferMemory, VkSharingMode sharingMode = VK_SHARING_MODE_EXCLUSIVE);

	void _CreateBufferAndCopy(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, const void * srcData, VkBuffer & buffer, VkDeviceMemory & bufferMemory);
	void _CopyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size, VkDeviceSize srcOffset = 0, VkDeviceSize dstOffset = 0);

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
	void _CleanupDropedTextures(bool all = false);
	void _CleanupShaderPrograms();
	void _Cleanup();

	CShaderProgram *_GetShader(const char *shader);

	bool _SimpleAcquireNextImage(uint32_t & imageIndex);
	void _SimpleQueueSubmit(VkSemaphore & waitSemaphore, VkSemaphore & signalSemaphore, VkCommandBuffer & commandBuffer);
	void _SimplePresent(VkSemaphore & waitSemaphore, uint32_t imageIndex);

	bool _UpdateBeforeDraw(float dt);

	void _RecreateSwapChain();
	void _CopyBufferToImage(VkImage dstImage, VkBuffer srcBuffer, VkBufferImageCopy & region, VkImageSubresourceRange & subresourceRange);

	bool m_enablePiplineStatistic;
	void _SetupPipelineStatistic();
	void _GetPipelineStatistic();
	VkQueryPool m_pipelineStatisticsQueryPool;
	std::vector<uint64_t>m_pipelineStats;

	uint32_t _mipLevels(uint32_t x, uint32_t y) { return static_cast<uint32_t>(floor(log2(std::max(x, y))) + 1); }

public:
	OutputWindow(uint32_t wnd);
	~OutputWindow() { _Cleanup(); }
	void Init();
	void Prepare(VkInstance _instance, GLFWwindow* _window, const VkAllocationCallbacks* _allocator);
	
	CDescriptorSetsMananger *GetDescriptorSetsManager() { return &descriptorSetsManager; }

	void Close(GLFWwindow *wnd = nullptr);

	void _UpdateUniformBuffer();

	void Update(float dt);
	void PrepareShader(CShaderProgram * sh);
	bool Exist() { return device != VK_NULL_HANDLE; }
	void BufferRecreationNeeded() { updateFlags.commandBuffers[FORWARD] = true; updateFlags.commandBuffers[DEFERRED] = true; }
	bool IsValid() { return window != nullptr; }
	bool IsBufferFeatureSupported(VkFormat format, VkFormatFeatureFlagBits features);

	void CopyBuffer(VkBuffer dstBuf, VkDeviceSize offset, VkDeviceSize size, void *srcData);
	void CopyImage(VkImage dstImage, Image * srcImage);


	void _iglfw_cursorPosition(double x, double y);
	void _iglfw_cursorEnter(bool v);
	void _iglfw_mouseButton(int button, int action, int mods);
	void _iglfw_scroll(double x, double y);
	void _iglfw_key(int key, int scancode, int action, int mods);
	void _iglfw_character(unsigned int codepoint);
	void _iglfw_resize(int width, int height);
	void _iglfw_close();

	template<typename T> void vkDestroy(T &v) { if (v) { vkDestroyDevice(v, allocator); v = VK_NULL_HANDLE; } }

#ifdef Define_vkDestroy
#undef Define_vkDestroy
#endif
#define Define_vkDestroy(t) template<> void vkDestroy(Vk##t &v) { if (v) { vkDestroy##t(device, v, allocator); v = VK_NULL_HANDLE; } }
	Define_vkDestroy(Pipeline);
	Define_vkDestroy(PipelineCache);
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
	Define_vkDestroy(QueryPool);
	template<> void vkDestroy(VkSurfaceKHR &v) { if (v) { vkDestroySurfaceKHR(instance, v, allocator); v = VK_NULL_HANDLE; } }
	template<> void vkDestroy(FrameBufferAttachment &v) { vkDestroy(v.image); vkDestroy(v.view); vkFree(v.memory); }

	void vkFree(VkDeviceMemory &v) { if (v) { vkFreeMemory(device, v, allocator); v = nullptr; } }
	void vkFree(VkCommandPool &v1, std::vector<VkCommandBuffer> &v2) { if (v1 && v2.size()) { vkFreeCommandBuffers(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }
	void vkFree(VkDescriptorPool &v1, std::vector<VkDescriptorSet> &v2) { if (v1 && v2.size()) { vkFreeDescriptorSets(device, v1, static_cast<uint32_t>(v2.size()), v2.data()); v2.clear(); } }

	std::vector<CShaderProgram*> forwardRenderingShaders;
	CStringHastable<CShaderProgram> shaders;
	basic_array<ObjectInfo> objects;

	ObjectInfo * AddObject(const char * mesh, const char * shader);
	void AddModel(const char * name);

	CShaderProgram *AddShader(const char * shader);
	CShaderProgram *ReloadShader(const char *shader);
	void GetShaderParams(const char * shader, Properties **params);
	void GetObjectParams(uint32_t idx, Properties ** props);
	void UpdateDrawCommands();

	CTexture2d * _AddOrUpdateTexture(const char * textureName, IResource * textureRes);
	void AddTexture(void * propVal, const char * textureName, IResource * textureRes = nullptr);
	void UpdateTexture(const char * textureName, IResource *textureRes);
	void _MarkDescriptorSetsInvalid(VkDescriptorImageInfo *pDII);

	// ------------------------ camera stuffs -------------------

	void SetCamera(const BAMS::PSET_CAMERA * cam);
	void _UpdateCamera();

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
	CTexture2d *pDefaultEmptyTexture;

	struct DropedTexture {
		VkSampler sampler = VK_NULL_HANDLE;
		VkImage image = VK_NULL_HANDLE;
		VkDeviceMemory memory = VK_NULL_HANDLE;
		VkImageView view = VK_NULL_HANDLE;
		uint64_t frame;

		DropedTexture(IMemoryAllocator *alloc, const CTexture2d *tex, uint64_t frameCounter) :
			sampler(tex->sampler),
			image(tex->image),
			memory(tex->memory),
			view(tex->view),
			frame(frameCounter)
		{}
	};

	template<> void vkDestroy(DropedTexture &tex)
	{
		vkDestroy(tex.view);
		vkDestroy(tex.image);
		vkDestroy(tex.sampler);
		vkFree(tex.memory);
	}
	queue<DropedTexture> dropedTextures;


	// ------------------------- for gui ----------------
	VkImGui *imGui;

	friend CDescriptorSetsMananger;
	friend CShaderProgram;
	friend CTexture2d;
	friend VkImGui;
	friend VkTools;
};
