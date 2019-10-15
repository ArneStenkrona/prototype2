#include "input.h"

#include <cassert>

int input::getKey(int /*keyCode*/) {
    //assert(keyCode < GLFW_KEY_LAST);
    //return glfwGetKey(window, GLFW_KEY_W);
    return 1;
}

void input::getCursorPos(double* /*xpos*/, double* /*ypos*/) {
    //glfwGetCursorPos(window, &xpos, &ypos);
}