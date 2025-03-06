#pragma once

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "common.h"
#include "window.h"
#include "renderer/vk_renderer.h"

class Engine {
public:

	void run();
	
	GLFWwindow* window;

private:

};