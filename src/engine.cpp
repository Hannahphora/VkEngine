#include "engine.h"

void Engine::run() {

    initWindow(800, 600, "Window");
    
    Renderer renderer = { .window = window };
    renderer.init();

    while (!glfwWindowShouldClose(window)) {

		renderer.draw();

        glfwPollEvents();
    }

    renderer.cleanup();
}

void Engine::initWindow(int width, int height, const char* title) {

	if (!glfwInit()) {
		fmt::print("error: failed to init glfw\n");
		abort();
	}

	glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

	window = glfwCreateWindow(width, height, title, nullptr, nullptr);
	if (!window) {
		fmt::print("error: failed to create window\n");
		glfwTerminate();
		abort();
	}

}