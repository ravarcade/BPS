#pragma once

class CCamera
{
	PSET_CAMERA current;
	glm::vec3 pos, at, eye;
	POINT mouseCursorPosition;
	GLFWwindow* window;
public:
	
	void init(GLFWwindow* wnd);
	void update(float dt);
	void set(const BAMS::PSET_CAMERA *cam);
	PSET_CAMERA *get();
};
