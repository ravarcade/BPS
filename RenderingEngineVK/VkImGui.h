

// based on Sascha Willems Vulkan Example - imGui


class VkImGui : public iInputCallback
{
private:
	// Vulkan resources for rendering the UI
	VkBuffer vertexBuf;
	VkDeviceMemory vertexMem;
	VkBuffer indexBuf;
	VkDeviceMemory indexMem;
	void *vertexData;
	void *indexData;
	int32_t vertexCount = 0;
	int32_t indexCount = 0;
	float *pScale;
	float *pTranslate;
	CTexture2d *fontTexture; // all below things are inside fontTexture object


	//VkPipelineCache pipelineCache;
	//VkPipelineLayout pipelineLayout;
	//VkPipeline pipeline;
	//VkDescriptorPool descriptorPool;
	//VkDescriptorSetLayout descriptorSetLayout;
	//VkDescriptorSet descriptorSet;


public:
	OutputWindow *vk;
	// UI params are set via push constants
	struct PushConstBlock {
		glm::vec2 scale;
		glm::vec2 translate;
	} pushConstBlock;

	VkImGui(OutputWindow *_vk, float width, float height);
	~VkImGui();

	// Initialize styles, keys, etc.
	void init(float width, float height);

	// Initialize all Vulkan resources used by the ui
	void initResources();

	// Starts a new imGui frame and sets up windows and ui elements
	void newFrame();

	// Update vertex and index buffer containing the imGui elements when required
	void updateBuffers();

	// Draw current imGui frame into a command buffer
	void drawFrame(VkCommandBuffer commandBuffer);

	void _iglfw_cursorPosition(double x, double y);
	void _iglfw_cursorEnter(bool Enter);
	void _iglfw_mouseButton(int button, int action, int mods);;
	void _iglfw_scroll(double x, double y);

	void _iglfw_key(int key, int scancode, int action, int mods);
	void _iglfw_character(unsigned int codepoint);

	void _iglfw_resize(int width, int height) { };
	void _iglfw_close() {};


	// ==================================================================== GUI ===
	void initGui();
	void drawGui();

	void ShowProperties(Properties *prop, const char *name);

	bool m_showImGuiDemoWindow;
	std::string m_propName;
	Properties *m_prop;
	std::vector<IResource *> m_textureResources;
	std::vector<VkDescriptorImageInfo *> m_textureDescriptors;

	bool inputProperties(Properties *prop);
	bool selectTexture(const char *label, VkDescriptorImageInfo **pPropVal, IResource **outRes);
		// ==================================================================== Helpers ===
private:
	void _updateTextureData();
	int _getTexture(VkDescriptorImageInfo *val);

	// matrix decomposition
	struct decomposedMatricesCacheEntry
	{
		decomposedMatricesCacheEntry() : frame(-2) {}
		const float *f;
		float r[3];
		float s[3];
		float t[3];
		int frame;
	};
	std::vector<decomposedMatricesCacheEntry> m_decomposedMatricesCache;
	void _decomposeM(const float *f, float **r, float **s, float **t);
};

