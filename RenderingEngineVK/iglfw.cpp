#include "stdafx.h"

int iglfw::GLFW_LastErr = 0;
const char *iglfw::GLFW_LastErrStr = nullptr;

void iglfw::ErrorCallback(int err, const char* err_str)
{
	GLFW_LastErr = err;
	GLFW_LastErrStr = err_str;
}

iglfw::iglfw()
{
	memset(windows, 0, sizeof(windows));
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
			glfwDestroyWindow(wnd);
			wnd = nullptr;
		}
	}
}



GLFWwindow* iglfw::CreateWnd(int wnd, int width, int height)
{
	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
	windows[wnd] = glfwCreateWindow(width, height, "Rendering Engine: Vulkan", nullptr, nullptr);
	return windows[wnd];
}

void iglfw::CreateVKSurface(VkInstance instance, int wnd, const VkAllocationCallbacks* allocator, VkSurfaceKHR* surface)
{
	if (glfwCreateWindowSurface(instance, windows[wnd], allocator, surface) != VK_SUCCESS)
	{
		throw std::runtime_error("failed to create window surface!");
	}
}

GLFWwindow *iglfw::GetWnd(int wnd) 
{ 
	return windows[wnd]; 
}

void iglfw::GetWndSize(int wnd, int *width, int *height)
{
	glfwGetWindowSize(windows[wnd], width, height);
//	glfwGetFramebufferSize(windows[wnd], width, height);
}


iglfw glfw;