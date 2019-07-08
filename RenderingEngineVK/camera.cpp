#include "stdafx.h"
#include "..\3rdParty\ImGui\imgui.h"
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/constants.hpp>
#include <glm/gtc/matrix_inverse.hpp>

PSET_CAMERA defaultCam = {
	0, // first window
	{ 0.0f, 100.0f, 0.0f },
	{ 0, 0, 0 },
	{ 0, 0, 1},
	45.0f,   // fov
	1.0f,    // z-near
	1000.0f, // z-far
};

void CCamera::init(GLFWwindow* wnd) { current = defaultCam; window = wnd; }
void CCamera::set(const BAMS::PSET_CAMERA * cam) { current = *cam; }
PSET_CAMERA * CCamera::get() { return &current; }

using glm::vec3;
using glm::quat;
vec3 &asVec3(float (&v)[3]) { return *reinterpret_cast<vec3*>(&v); }

void CCamera::update(float dt)
{
	// mouse test
	if (false)
	{
		static uint32_t xbtn = 0, oxbtn;
		static int xmx, xmy, oxmx, xcnt = 0, xcnt2 = 0;
		oxbtn = xbtn;
		Tools::ReadMouse(&xmx, &xmy, &xbtn);
		if (oxbtn == 0 && xbtn == 1)
		{
			// click;
			xcnt = 0;
		}

		if (oxmx == 0 && xmx != 0)
		{
			xcnt2 = 0;
		}

		if (xcnt2 < 100)
			++xcnt2;
		if (xcnt < 100)
			++xcnt;
		static double xpos = 0, ypos = 0;
		double oxpos = xpos;
		glfwGetCursorPos(window, &xpos, &ypos);
		static double olddx = 0;
		double dx = xpos - oxpos;

		if (olddx == 0 && dx != 0)
		{
			TRACE("[[" << xcnt2 << "]]\n");
		}
		ImGuiIO& io = ImGui::GetIO();
		static int mx = 0, my = 100;
		static bool run = false;
		static int diff[10] = { 0 };
		static int diff2[10] = { 0 };
		static int diff3[10] = { 0 };
		static int i = 0;
		if (i < 10)
		{
			diff[i] = (int)io.MousePos.x;
			diff2[i] = (int)xpos;
			diff3[i] = xmx;
			++i;
			if (i == 10)
			{
				for (uint32_t i = 0; i < 10; ++i)
				{
//					TRACE(">> " << i << ":" << diff[i] << ", " << diff2[i] << "\n");
					TRACE(">> " << i << ":" << diff[i] << ", " << diff2[i] << ", " << diff3[i] << "\n");
				}
			}
		}
		if (io.MouseClicked[0])
		{
			run = true;
			TRACE("[" << xcnt << "]\n");
			i = 0;
		}
		if (io.MouseReleased[0])
		{
			run = false;
			mx = 0;
		}
		if (run) {
			SetCursorPos(mx, my);
			mx += 2;
		}
		return;
	}
	ImGuiIO& io = ImGui::GetIO();
	int mx = 0, my = 0;
	Tools::ReadMouse(&mx, &my); // mouse must be read every frame, otherwies value will be accumulate.
	if (!io.WantCaptureMouse)
	{
		if (io.MouseClicked[0])
		{
			GetCursorPos(&mouseCursorPosition);
		}

		if (io.MouseDown[0])
		{
			float mouseScale = 0.0025f; // *dt?

			vec3 dir = asVec3(current.lookAt) - asVec3(current.eye);
			dir = glm::normalize(dir);
			vec3 right = glm::cross(dir, asVec3(current.up));

			
			glm::mat4 R(1.0);
			R = glm::rotate(R, -float(mx)*mouseScale, asVec3(current.up));
			R = glm::rotate(R, -float(my)*mouseScale, right);

			dir = R * glm::vec4(dir,0);
			*reinterpret_cast<vec3 *>(&current.lookAt) = *reinterpret_cast<vec3 *>(&current.eye) + dir;
			SetCursorPos(mouseCursorPosition.x, mouseCursorPosition.y);
		}
	}

	if (!io.WantCaptureKeyboard)
	{
		float moveScale = 2.0f;

		vec3 dir = *reinterpret_cast<vec3 *>(&current.lookAt) - *reinterpret_cast<vec3 *>(&current.eye);
		glm::normalize(dir);
		vec3 right = glm::cross(dir, *reinterpret_cast<vec3 *>(&current.up));
		glm::normalize(right);

		if (io.KeysDown['W'])
		{
			*reinterpret_cast<vec3 *>(&current.eye) += dir * moveScale;
		}
		if (io.KeysDown['S'])
		{
			*reinterpret_cast<vec3 *>(&current.eye) -= dir * moveScale;
		}
		if (io.KeysDown['A'])
		{
			*reinterpret_cast<vec3 *>(&current.eye) -= right * moveScale;
		}
		if (io.KeysDown['D'])
		{
			*reinterpret_cast<vec3 *>(&current.eye) += right * moveScale;
			
		}
		*reinterpret_cast<vec3 *>(&current.lookAt) = *reinterpret_cast<vec3 *>(&current.eye) + dir;
		if (io.KeysDown['R'])
		{
			current = defaultCam;
		}

	}
}
