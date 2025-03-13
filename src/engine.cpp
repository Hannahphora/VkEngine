#include "engine.h"
#include <thread>
#include <chrono>

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>

void Engine::run() {

    initWindow(800, 600, "Window");
    
    renderer = new Renderer{ .window = window };
    renderer->init();

	registerInputActions(window);

    while (!glfwWindowShouldClose(window)) {
		// poll/process events
		glfwPollEvents();
		input->processActions();

		if (renderer->stopRendering) {
			std::this_thread::sleep_for(std::chrono::milliseconds(100));
			continue;
		}			

		renderer->draw();
    }

    renderer->cleanup();
	delete renderer;
	delete input;
	glfwTerminate();
}

void Engine::initWindow(int width, int height, const char* title) {

	glfwSetErrorCallback(glfw_error_callback);
	if (!glfwInit()) {
		fmt::print("error: failed to init glfw\n");
		exit(EXIT_FAILURE);
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

	monitor = glfwGetPrimaryMonitor();
	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		fmt::print(stderr, "error: failed to create window\n");
		glfwTerminate();
		exit(EXIT_FAILURE);
	}
	if (!glfwVulkanSupported()) {
        fmt::print(stderr, "error: glfw does not support vulkan\n");
        exit(EXIT_FAILURE);
    }

	glfwSetWindowUserPointer(window, this);
	input = new InputManager(window);
}

void Engine::registerInputActions(GLFWwindow* window) {
	// quit
	input->registerAction("Quit", Binding::key(GLFW_KEY_ESCAPE, GLFW_PRESS), [window]() {
		glfwSetWindowShouldClose(window, true);
	});

}