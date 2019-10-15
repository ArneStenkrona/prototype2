#include "input.h"

#include <cassert>

Input::Input(GLFWwindow* window) 
: _window(window),
  _dx(0.0),
  _dy(0.0)
{}

int Input::getKey(int keyCode) {
    assert(keyCode < GLFW_KEY_LAST);
    return glfwGetKey(_window, keyCode);
}

void Input::getCursorPos(double& xpos, double& ypos) {
    glfwGetCursorPos(_window, &xpos, &ypos);
}

void Input::getCursorDelta(double& dx, double& dy) {
    dx = _dx;
    dy = _dy;
}

void Input::update() {
    double x, y;
    glfwGetCursorPos(_window, &x, &y);
    _dx = x - _lastCursorX;
    _dy = y - _lastCursorY;
    _lastCursorX = x;
    _lastCursorY = y;
}