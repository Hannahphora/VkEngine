#pragma once

#include "GLFW/glfw3.h"
#include "cstdlib"
#include "fmt/core.h"

namespace Window {

	GLFWwindow* init(int width, int height, const char* title);

}