#include "input.h"

#include <cassert>

Input::Input() 
: _window(nullptr),
  _lastCursorX(0.0f),
  _lastCursorY(0.0f),
  _dx(0.0),
  _dy(0.0)
{
}

void Input::init(GLFWwindow* window) {
    _window = window;
    glfwSetInputMode(_window, GLFW_STICKY_MOUSE_BUTTONS, GLFW_TRUE);
}

int Input::getKey(int keyCode) {
    assert(keyCode < GLFW_KEY_LAST);
    return glfwGetKey(_window, keyCode);
}

int Input::getMouseButton(int mouseButton) {
    return glfwGetMouseButton(_window, mouseButton);
}

void Input::getCursorPos(double& xpos, double& ypos) {
    glfwGetCursorPos(_window, &xpos, &ypos);
}

void Input::getCursorDelta(double& dx, double& dy) {
    dx = _dx;
    dy = _dy;
}

void Input::update() {
    // update cursor delta
    double x, y;
    glfwGetCursorPos(_window, &x, &y);
    _dx = x - _lastCursorX;
    _dy = y - _lastCursorY;
    _lastCursorX = x;
    _lastCursorY = y;
    // lock/unlock cursor
    /*if (glfwGetKey(_window, GLFW_KEY_ESCAPE)) {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
        _dx = 0;
        _dy = 0;
    }
    int state = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);
    if (state == GLFW_PRESS)
    {
        glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }*/
}