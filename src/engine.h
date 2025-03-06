#pragma once
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include "renderer/vk_renderer.h"

class Engine {
public:
	void run();
	
	GLFWwindow* window;

private:
	void initWindow(int width, int height, const char* title);
};