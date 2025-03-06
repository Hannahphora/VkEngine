#include "engine.h"

void Engine::run() {

    window = Window::init(800, 600, "Window");
    
    Renderer renderer = { .window = window };
    renderer.init();

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
    }

    renderer.cleanup();
}