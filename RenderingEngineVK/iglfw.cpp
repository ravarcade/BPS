#include "stdafx.h"

int iglfw::GLFW_LastErr = 0;
const char *iglfw::GLFW_LastErrStr = nullptr;


void window_cursorPosition_callback(GLFWwindow *wnd, double x, double y) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_cursorPosition(x, y); }
void window_cursorEnter_callback(GLFWwindow *wnd, int enter) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_cursorEnter(enter == GLFW_TRUE); }
void window_mouseButton_callback(GLFWwindow *wnd, int button, int action, int mods) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_mouseButton(button, action, mods); }
void window_scroll_callback(GLFWwindow *wnd, double x, double y) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_scroll(x,y); }
void windw_key_callback(GLFWwindow* wnd, int key, int scancode, int action, int mods) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_key(key, scancode, action, mods); }
void window_character_callback(GLFWwindow* wnd, unsigned int codepoint) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_character(codepoint); }
void window_resize_callback(GLFWwindow *wnd, int w, int h) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_resize(w,h); }
void window_close_callback(GLFWwindow* wnd) { reinterpret_cast<iInputCallback *>(glfwGetWindowUserPointer(wnd))->_iglfw_close(); }

void iglfw::ErrorCallback(int err, const char* err_str)
{
	GLFW_LastErr = err;
	GLFW_LastErrStr = err_str;
}

iglfw::iglfw()
{
	windows.clear();
}

void iglfw::Init()
{
	glfwSetErrorCallback(ErrorCallback);
	glfwInit();
}

void iglfw::Cleanup()
{
	for (auto &wnd : windows)
	{
		if (wnd)
			glfwDestroyWindow(wnd);
	}
	glfwTerminate();
}

void iglfw::Update(float dt)
{
	for (auto &wnd : windows)
	{
		if (!wnd)
			continue;
	
		if (!glfwWindowShouldClose(wnd))
		{
			glfwPollEvents();
		}
		else
		{
			// tell re that window should be closed
			re.CloseWnd(wnd);
			glfwDestroyWindow(wnd);
			wnd = nullptr;
		}
	}
}

void iglfw::SetWindowUserInput(GLFWwindow *window, iInputCallback *inputCallbacks)
{
	glfwSetCursorPosCallback(window, window_cursorPosition_callback);
//	glfwSetCursorEnterCallback(window, window_cursorEnter_callback);
	glfwSetMouseButtonCallback(window, window_mouseButton_callback);
	glfwSetScrollCallback(window, window_scroll_callback);
	glfwSetKeyCallback(window, windw_key_callback);
	glfwSetCharCallback(window, window_character_callback);
	glfwSetWindowCloseCallback(window, window_close_callback);
	glfwSetWindowSizeCallback(window, window_resize_callback);
	glfwSetInputMode(window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
	glfwSetInputMode(window, GLFW_STICKY_KEYS, GLFW_TRUE);
//	glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN);
	glfwSetWindowUserPointer(window, inputCallbacks);	
}

GLFWwindow* iglfw::CreateWnd(int wnd, int width, int height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	auto window = glfwCreateWindow(width, height, "Rendering Engine: Vulkan", nullptr, nullptr);
	glfwSetWindowCloseCallback(window, window_close_callback);

	windows.push_back(window);

	return window;
}

void iglfw::CreateVKSurface(VkInstance instance, int wnd, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, windows[wnd], allocator, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

void iglfw::GetWndSize(GLFWwindow *wnd, int *width, int *height)
{
	glfwGetWindowSize(wnd, width, height);
}


iglfw glfw;