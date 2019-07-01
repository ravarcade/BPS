
class iInputCallback 
{
public:
	virtual void _iglfw_cursorPosition(double x, double y) = 0;
	virtual void _iglfw_cursorEnter(bool Enter) = 0;
	virtual void _iglfw_mouseButton(int button, int action, int mods) = 0;
	virtual void _iglfw_scroll(double x, double y) = 0;
	virtual void _iglfw_key(int key, int scancode, int action, int mods) = 0;
	virtual void _iglfw_character(unsigned int codepoint) = 0;
	virtual void _iglfw_resize(int width, int height) = 0;
	virtual void _iglfw_close() = 0;
};

class iglfw 
{
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
	void SetWindowUserInput(GLFWwindow *wnd, iInputCallback *inputCallbacks);
};

extern iglfw glfw;