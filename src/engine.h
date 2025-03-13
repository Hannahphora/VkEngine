#pragma once
#include <GLFW/glfw3.h>
#include <fmt/core.h>

#include "renderer/vk_renderer.h"
#include "input.h"

static void glfw_error_callback(int error, const char* description) {
    fmt::print(stderr, "glfw error %d: %s\n", error, description);
}

class Engine {
public:
	void run();
	
	GLFWwindow* window = nullptr;
	GLFWmonitor* monitor = nullptr;
	InputManager* input = nullptr;
	Renderer* renderer = nullptr;

private:
	void initWindow(int width, int height, const char* title);
	void registerInputActions(GLFWwindow* window);
};