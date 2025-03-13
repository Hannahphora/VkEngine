#pragma once
#include <glfw/glfw3.h>

#include <functional>
#include <unordered_map>
#include <vector>
#include <string>

constexpr int MAX_KEYS = 350;
constexpr int MAX_MOUSE_BUTTONS = 8;

struct Binding {
    enum class Type {
        Key,
        MouseButton,
        MouseMove,
        MouseScrollUp,
        MouseScrollDown,
        Composite
    } type = {};
    int code = 0;
    int event = 0; // GLFW_PRESS, GLFW_RELEASE, GLFW_REPEAT
    std::vector<Binding> subBindings;

    // constructors
    static Binding key(int code, int event) { return Binding{ Type::Key, code, event }; }
    static Binding mouseButton(int code, int event) { return Binding{ Type::MouseButton, code, event }; }
    static Binding mouseMove() { return Binding{ Type::MouseMove }; }
    static Binding scrollUp() { return Binding{ Type::MouseScrollUp }; }
    static Binding scrollDown() { return Binding{ Type::MouseScrollDown }; }
    static Binding composite(std::initializer_list<Binding> bindings) {
        Binding b;
        b.type = Type::Composite;
        b.subBindings.assign(bindings.begin(), bindings.end());
        return b;
    }
};

struct InputAction {
    bool active = true;
    std::string id;
    std::vector<Binding> bindings;
    std::vector<std::function<void()>> callbacks;
};

class InputManager {
public:
    
    InputManager(GLFWwindow* wnd);

    // update state funcs (called from glfw callbacks)

    void updateKeyState(int code, int event);
    void updateMouseButtonState(int code, int event);
    void updateMousePos(double xpos, double ypos);
    void updateMouseScroll(double xoffset, double yoffset);

    // direct callbacks

    void addKeyCallback(std::function<void(int code, int event)> callback);
    void addMouseButtonCallback(std::function<void(int code, int event)> callback);
    void addMouseMoveCallback(std::function<void(double xpos, double ypos)> callback);
    void addMouseScrollCallback(std::function<void(double yoffset)> callback);

    // action/binding funcs
    // NOTE: for any funcs that return an int, 0 = success, !0 = failure

    void processActions();
    int registerAction(const std::string& id);
    int registerAction(const std::string& id, const Binding& binding, std::function<void()> callback);
    int addActionBinding(const std::string& id, const Binding& binding);
    int addActionCallback(const std::string& id, std::function<void()> callback);
    int setActionActive(const std::string& id, bool active);

    // getters for input states

    bool isKeyPressed(int key) const;
    bool isKeyHeld(int key) const;
    bool isKeyReleased(int key) const;

    bool isMouseButtonPressed(int button) const;
    bool isMouseButtonHeld(int button) const;
    bool isMouseButtonReleased(int button) const;

    double getMouseX() const;
    double getMouseY() const;

    double getScrollX() const;
    double getScrollY() const;

private:

    GLFWwindow* window;
    std::unordered_map<std::string, InputAction> actions;

    // direct callbacks
    std::vector<std::function<void(int code, int event)>> keyCallbacks;
    std::vector<std::function<void(int code, int event)>> mouseButtonCallbacks;
    std::vector<std::function<void(double xpos, double ypos)>> mouseMoveCallbacks;
    std::vector<std::function<void(double yoffset)>> mouseScrollCallbacks;

    // key and mouse button state
    bool keyState[MAX_KEYS] = { false };
    bool prevKeyState[MAX_KEYS] = { false };
    bool mbState[MAX_MOUSE_BUTTONS] = { false };
    bool prevMBState[MAX_MOUSE_BUTTONS] = { false };

    // mouse pos state
    double mouseX = 0.0, mouseY = 0.0;
    double prevMouseX = 0.0, prevMouseY = 0.0;

    // scroll state (accumulated per frame)
    double scrollX = 0.0, scrollY = 0.0;

};
