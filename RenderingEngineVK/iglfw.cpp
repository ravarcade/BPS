#include "stdafx.h"

int iglfw::GLFW_LastErr = 0;
const char *iglfw::GLFW_LastErrStr = nullptr;

void window_close_callback(GLFWwindow* window)
{
	
}

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

void iglfw::SetWindowSizeCallback(GLFWwindow *wnd, void(*OnWindowResizeCallback)(GLFWwindow *wnd, int width, int height))
{
	glfwSetWindowSizeCallback(wnd, OnWindowResizeCallback);
}

void *iglfw::GetWindowUserPointer(GLFWwindow *wnd)
{
	return glfwGetWindowUserPointer(wnd);
}

void iglfw::SetWindowUserPointer(GLFWwindow *wnd, void *userPointer)
{
	glfwSetWindowUserPointer(wnd, userPointer);
}

iglfw glfw;