#pragma once
#include "GLFW/glfw3.h"
#include "common.h"

namespace Window {
	GLFWwindow* init(int width, int height, const char* title);
}