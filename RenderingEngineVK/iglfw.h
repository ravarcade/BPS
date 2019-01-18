
class iglfw {
	static void ErrorCallback(int err, const char* err_str);

public:
	static int GLFW_LastErr;
	static const char *GLFW_LastErrStr;

	std::vector<GLFWwindow*> windows;
	iglfw();

	void Init();
	void Cleanup();
	void Update(float dt);

	// --------------------------------------------------

	GLFWwindow *CreateWnd(int wnd, int width, int height);
	void CreateVKSurface(VkInstance instance, int wnd, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface);
	void GetWndSize(GLFWwindow *wnd, int *width, int *height);

};

extern iglfw glfw;