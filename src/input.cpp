#include "input.h"
#include "engine.h"

static void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods) {
    Engine* engine = (Engine*)glfwGetWindowUserPointer(window);
    engine->input->updateKeyState(key, action);
}

static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    Engine* engine = (Engine*)glfwGetWindowUserPointer(window);
    engine->input->updateMouseButtonState(button, action);
}

static void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    Engine* engine = (Engine*)glfwGetWindowUserPointer(window);
    engine->input->updateMousePos(xpos, ypos);
}

static void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) {
    Engine* engine = (Engine*)glfwGetWindowUserPointer(window);
    engine->input->updateMouseScroll(xoffset, yoffset);
}

// constructor/initialisation

InputManager::InputManager(GLFWwindow* wnd) : window(wnd) {
    glfwSetKeyCallback(wnd, key_callback);
    glfwSetMouseButtonCallback(wnd, mouse_button_callback);
    glfwSetCursorPosCallback(wnd, cursor_position_callback);
    glfwSetScrollCallback(wnd, scroll_callback);
}

//  update state funcs (called from glfw callbacks)

void InputManager::updateKeyState(int code, int event) {
    prevKeyState[code] = keyState[code];
    keyState[code] = (event != GLFW_RELEASE);
    for (const auto& callback : keyCallbacks)
        callback(code, event);
}

void InputManager::updateMouseButtonState(int code, int event) {
    prevMBState[code] = mbState[code];
    mbState[code] = (event != GLFW_RELEASE);
    for (const auto& callback : mouseButtonCallbacks)
        callback(code, event);
}

void InputManager::updateMousePos(double xpos, double ypos) {
    mouseX = xpos;
    mouseY = ypos;
    for (const auto& callback : mouseMoveCallbacks)
        callback(xpos, ypos);
}

void InputManager::updateMouseScroll(double xoffset, double yoffset) {
    scrollX += xoffset;
    scrollY += yoffset;
    for (const auto& callback : mouseScrollCallbacks)
        callback(yoffset);
}

// direct callback funcs

void InputManager::addKeyCallback(std::function<void(int code, int event)> callback) {
    keyCallbacks.emplace_back(callback);
}

void InputManager::addMouseButtonCallback(std::function<void(int code, int event)> callback) {
    mouseButtonCallbacks.emplace_back(callback);
}

void InputManager::addMouseMoveCallback(std::function<void(double xpos, double ypos)> callback) {
    mouseMoveCallbacks.emplace_back(callback);
}

void InputManager::addMouseScrollCallback(std::function<void(double yoffset)> callback) {
    mouseScrollCallbacks.emplace_back(callback);
}

// action/binding funcs

int InputManager::registerAction(const std::string& id) {
    if (actions.count(id)) return 1;
    actions[id] = InputAction{ .id = id };
    return 0;
}

int InputManager::registerAction(const std::string& id, const Binding& binding, std::function<void()> callback) {
    if (actions.count(id)) return 1;
    actions[id] = InputAction{ .id = id };
    addActionBinding(id, binding);
    addActionCallback(id, callback);
    return 0;
}

int InputManager::addActionBinding(const std::string& id, const Binding& binding) {
    if (!actions.count(id)) return 1;
    actions[id].bindings.emplace_back(binding);
    return 0;
}

int InputManager::addActionCallback(const std::string& id, std::function<void()> callback) {
    if (!actions.count(id)) return 1;
    actions[id].callbacks.emplace_back(callback);
    return 0;
}

int InputManager::setActionActive(const std::string& id, bool active) {
    if (!actions.count(id)) return 1;
    actions[id].active = active;
    return 0;
}

void InputManager::processActions() {
    double epsilon = std::numeric_limits<double>::epsilon();

    // check trigger for keys and mouse buttons
    auto checkTrigger = [](bool current, bool previous, int event) -> bool {
        switch (event) {
        case GLFW_PRESS:   return current && !previous;
        case GLFW_REPEAT:  return current && previous;
        case GLFW_RELEASE: return !current && previous;
        default:           return false;
        }
    };

    // recursive lambda to check binding, including composites
    std::function<bool(const Binding&)> checkBinding;
    checkBinding = [&](const Binding& binding) -> bool {
        if (binding.type == Binding::Type::Composite) {
            for (const auto& sub : binding.subBindings) 
                if (!checkBinding(sub))
                    return false;
            return true;
        }

        switch (binding.type) {
        case Binding::Type::Key:
            return checkTrigger(keyState[binding.code], prevKeyState[binding.code], binding.event);
        case Binding::Type::MouseButton:
            return checkTrigger(mbState[binding.code], prevMBState[binding.code], binding.event);
        case Binding::Type::MouseMove:
            return (std::abs(mouseX - prevMouseX) > epsilon) || (std::abs(mouseY - prevMouseY) > epsilon);
        case Binding::Type::MouseScrollUp:
            return scrollY > epsilon;
        case Binding::Type::MouseScrollDown:
            return scrollY < -epsilon;
        default:
            return false;
        }
    };

    // iterate over actions
    for (const auto& [actionID, action] : actions) {
        if (!action.active) continue;

        // iterate over bindings, if any are met then run callbacks
        if ([&]() -> bool {
            for (const auto& binding : action.bindings)
                if (checkBinding(binding))
                    return true;
            return false;
        }()) {
            for (const auto& callback : action.callbacks)
                callback();
        }
    }

    // update prev states
    memcpy(prevKeyState, keyState, sizeof(keyState));
    memcpy(prevMBState, mbState, sizeof(mbState));
    prevMouseX = mouseX;
    prevMouseY = mouseY;
    scrollX = 0.0;
    scrollY = 0.0;
}

// getters for input states

bool InputManager::isKeyPressed(int key) const {
    return keyState[key] && !prevKeyState[key];
}

bool InputManager::isKeyHeld(int key) const {
    return keyState[key] && prevKeyState[key];
}

bool InputManager::isKeyReleased(int key) const {
    return !keyState[key] && prevKeyState[key];
}

bool InputManager::isMouseButtonPressed(int button) const {
    return mbState[button] && !prevMBState[button];
}

bool InputManager::isMouseButtonHeld(int button) const {
    return mbState[button] && prevMBState[button];
}

bool InputManager::isMouseButtonReleased(int button) const {
    return !mbState[button] && prevMBState[button];
}

double InputManager::getMouseX() const {
    return mouseX;
}

double InputManager::getMouseY() const {
    return mouseY;
}

double InputManager::getScrollX() const {
    return scrollX;
}

double InputManager::getScrollY() const {
    return scrollY;
}