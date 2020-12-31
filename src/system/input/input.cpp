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

    // Set screen/pixel coordinate mapping
    int widthW, heightW;
    glfwGetWindowSize(window, &widthW, &heightW);
    int widthB, heightB;
    glfwGetFramebufferSize(window, &widthB, &heightB);
    scaleX = float(widthB) / float(widthW);
    scaleY = float(heightB) / float(heightW);

    for (size_t i = 0; i < currentKeyboardState.size(); i++) {
        currentKeyboardState[i] = false;
        previousKeyboardState[i] = false;
    }
}

// int Input::getKey(int keyCode) {
//     assert(keyCode < GLFW_KEY_LAST);
//     return glfwGetKey(_window, keyCode);
// }

bool Input::getKeyPress(INPUT_KEY keyCode) {
    return currentKeyboardState[keyCode];
}
bool Input::getKeyDown(INPUT_KEY keyCode) {
    return currentKeyboardState[keyCode] && !previousKeyboardState[keyCode];
}
bool Input::getKeyUp(INPUT_KEY keyCode) {
    return !currentKeyboardState[keyCode] && previousKeyboardState[keyCode];
}

void Input::getCursorPos(double& xpos, double& ypos) {
    glfwGetCursorPos(_window, &xpos, &ypos);
    xpos *= scaleX;
    ypos *= scaleY;
}

void Input::getCursorDelta(double& dx, double& dy) {
    dx = _dx;
    dy = _dy;
}

void Input::update(bool enableCapture) {
    // update cursor delta
    double x, y;
    getCursorPos(x, y);
    _dx = x - _lastCursorX;
    _dy = y - _lastCursorY;
    _lastCursorX = x;
    _lastCursorY = y;
    // lock/unlock cursor
    if (enableCapture) {
        if (glfwGetKey(_window, GLFW_KEY_ESCAPE)) {
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            _dx = 0;
            _dy = 0;
        }
        int state = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT);
        if (state == GLFW_PRESS)
        {
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
        }
    } else {
        if (glfwGetInputMode(_window, GLFW_CURSOR) == GLFW_CURSOR_DISABLED) {
            glfwSetInputMode(_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            _dx = 0;
            _dy = 0;
        }
    }
    updateKeyboardState();
}

void Input::updateKeyboardState() {
    previousKeyboardState = currentKeyboardState;
    currentKeyboardState[INPUT_KEY::KEY_UNKNOWN] = glfwGetKey(_window, GLFW_KEY_UNKNOWN) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_SPACE] = glfwGetKey(_window, GLFW_KEY_SPACE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_APOSTROPHE] = glfwGetKey(_window, GLFW_KEY_APOSTROPHE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_COMMA] = glfwGetKey(_window, GLFW_KEY_COMMA) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_MINUS] = glfwGetKey(_window, GLFW_KEY_MINUS) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_PERIOD] = glfwGetKey(_window, GLFW_KEY_PERIOD) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_SLASH] = glfwGetKey(_window, GLFW_KEY_SLASH) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_0] = glfwGetKey(_window, GLFW_KEY_0) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_1] = glfwGetKey(_window, GLFW_KEY_1) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_2] = glfwGetKey(_window, GLFW_KEY_2) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_3] = glfwGetKey(_window, GLFW_KEY_3) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_4] = glfwGetKey(_window, GLFW_KEY_4) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_5] = glfwGetKey(_window, GLFW_KEY_5) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_6] = glfwGetKey(_window, GLFW_KEY_6) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_7] = glfwGetKey(_window, GLFW_KEY_7) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_8] = glfwGetKey(_window, GLFW_KEY_8) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_9] = glfwGetKey(_window, GLFW_KEY_9) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_SEMICOLON] = glfwGetKey(_window, GLFW_KEY_SEMICOLON) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_EQUAL] = glfwGetKey(_window, GLFW_KEY_EQUAL) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_A] = glfwGetKey(_window, GLFW_KEY_A) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_B] = glfwGetKey(_window, GLFW_KEY_B) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_C] = glfwGetKey(_window, GLFW_KEY_C) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_D] = glfwGetKey(_window, GLFW_KEY_D) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_E] = glfwGetKey(_window, GLFW_KEY_E) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F] = glfwGetKey(_window, GLFW_KEY_F) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_G] = glfwGetKey(_window, GLFW_KEY_G) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_H] = glfwGetKey(_window, GLFW_KEY_H) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_I] = glfwGetKey(_window, GLFW_KEY_I) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_J] = glfwGetKey(_window, GLFW_KEY_J) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_K] = glfwGetKey(_window, GLFW_KEY_K) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_L] = glfwGetKey(_window, GLFW_KEY_L) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_M] = glfwGetKey(_window, GLFW_KEY_M) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_N] = glfwGetKey(_window, GLFW_KEY_N) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_O] = glfwGetKey(_window, GLFW_KEY_O) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_P] = glfwGetKey(_window, GLFW_KEY_P) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_Q] = glfwGetKey(_window, GLFW_KEY_Q) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_R] = glfwGetKey(_window, GLFW_KEY_R) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_S] = glfwGetKey(_window, GLFW_KEY_S) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_T] = glfwGetKey(_window, GLFW_KEY_T) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_U] = glfwGetKey(_window, GLFW_KEY_U) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_V] = glfwGetKey(_window, GLFW_KEY_V) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_W] = glfwGetKey(_window, GLFW_KEY_W) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_X] = glfwGetKey(_window, GLFW_KEY_X) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_Y] = glfwGetKey(_window, GLFW_KEY_Y) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_Z] = glfwGetKey(_window, GLFW_KEY_Z) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT_BRACKET] = glfwGetKey(_window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_BACKSLASH] = glfwGetKey(_window, GLFW_KEY_BACKSLASH) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT_BRACKET] = glfwGetKey(_window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_GRAVE_ACCENT] = glfwGetKey(_window, GLFW_KEY_GRAVE_ACCENT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_WORLD_1] = glfwGetKey(_window, GLFW_KEY_WORLD_1) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_WORLD_2] = glfwGetKey(_window, GLFW_KEY_WORLD_2) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_ESCAPE] = glfwGetKey(_window, GLFW_KEY_ESCAPE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_ENTER] = glfwGetKey(_window, GLFW_KEY_ENTER) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_TAB] = glfwGetKey(_window, GLFW_KEY_TAB) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_BACKSPACE] = glfwGetKey(_window, GLFW_KEY_BACKSPACE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_INSERT] = glfwGetKey(_window, GLFW_KEY_INSERT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_DELETE] = glfwGetKey(_window, GLFW_KEY_DELETE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT] = glfwGetKey(_window, GLFW_KEY_RIGHT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT] = glfwGetKey(_window, GLFW_KEY_LEFT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_DOWN] = glfwGetKey(_window, GLFW_KEY_DOWN) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_UP] = glfwGetKey(_window, GLFW_KEY_UP) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_PAGE_UP] = glfwGetKey(_window, GLFW_KEY_PAGE_UP) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_PAGE_DOWN] = glfwGetKey(_window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_HOME] = glfwGetKey(_window, GLFW_KEY_HOME) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_END] = glfwGetKey(_window, GLFW_KEY_END) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_CAPS_LOCK] = glfwGetKey(_window, GLFW_KEY_CAPS_LOCK) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_SCROLL_LOCK] = glfwGetKey(_window, GLFW_KEY_SCROLL_LOCK) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_NUM_LOCK] = glfwGetKey(_window, GLFW_KEY_NUM_LOCK) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_PRINT_SCREEN] = glfwGetKey(_window, GLFW_KEY_PRINT_SCREEN) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_PAUSE] = glfwGetKey(_window, GLFW_KEY_PAUSE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F1] = glfwGetKey(_window, GLFW_KEY_F1) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F2] = glfwGetKey(_window, GLFW_KEY_F2) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F3] = glfwGetKey(_window, GLFW_KEY_F3) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F4] = glfwGetKey(_window, GLFW_KEY_F4) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F5] = glfwGetKey(_window, GLFW_KEY_F5) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F6] = glfwGetKey(_window, GLFW_KEY_F6) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F7] = glfwGetKey(_window, GLFW_KEY_F7) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F8] = glfwGetKey(_window, GLFW_KEY_F8) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F9] = glfwGetKey(_window, GLFW_KEY_F9) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F10] = glfwGetKey(_window, GLFW_KEY_F10) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F11] = glfwGetKey(_window, GLFW_KEY_F11) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F12] = glfwGetKey(_window, GLFW_KEY_F12) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F13] = glfwGetKey(_window, GLFW_KEY_F13) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F14] = glfwGetKey(_window, GLFW_KEY_F14) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F15] = glfwGetKey(_window, GLFW_KEY_F15) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F16] = glfwGetKey(_window, GLFW_KEY_F16) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F17] = glfwGetKey(_window, GLFW_KEY_F17) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F18] = glfwGetKey(_window, GLFW_KEY_F18) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F19] = glfwGetKey(_window, GLFW_KEY_F19) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F20] = glfwGetKey(_window, GLFW_KEY_F20) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F21] = glfwGetKey(_window, GLFW_KEY_F21) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F22] = glfwGetKey(_window, GLFW_KEY_F22) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F23] = glfwGetKey(_window, GLFW_KEY_F23) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F24] = glfwGetKey(_window, GLFW_KEY_F24) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_F25] = glfwGetKey(_window, GLFW_KEY_F25) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_0] = glfwGetKey(_window, GLFW_KEY_KP_0) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_1] = glfwGetKey(_window, GLFW_KEY_KP_1) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_2] = glfwGetKey(_window, GLFW_KEY_KP_2) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_3] = glfwGetKey(_window, GLFW_KEY_KP_3) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_4] = glfwGetKey(_window, GLFW_KEY_KP_4) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_5] = glfwGetKey(_window, GLFW_KEY_KP_5) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_6] = glfwGetKey(_window, GLFW_KEY_KP_6) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_7] = glfwGetKey(_window, GLFW_KEY_KP_7) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_8] = glfwGetKey(_window, GLFW_KEY_KP_8) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_9] = glfwGetKey(_window, GLFW_KEY_KP_9) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_DECIMAL] = glfwGetKey(_window, GLFW_KEY_KP_DECIMAL) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_DIVIDE] = glfwGetKey(_window, GLFW_KEY_KP_DIVIDE) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_MULTIPLY] = glfwGetKey(_window, GLFW_KEY_KP_MULTIPLY) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_SUBTRACT] = glfwGetKey(_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_ADD] = glfwGetKey(_window, GLFW_KEY_KP_ADD) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_ENTER] = glfwGetKey(_window, GLFW_KEY_KP_ENTER) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_KP_EQUAL] = glfwGetKey(_window, GLFW_KEY_KP_EQUAL) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT_SHIFT] = glfwGetKey(_window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT_CONTROL] = glfwGetKey(_window, GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT_ALT] = glfwGetKey(_window, GLFW_KEY_LEFT_ALT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_LEFT_SUPER] = glfwGetKey(_window, GLFW_KEY_LEFT_SUPER) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT_SHIFT] = glfwGetKey(_window, GLFW_KEY_RIGHT_SHIFT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT_CONTROL] = glfwGetKey(_window, GLFW_KEY_RIGHT_CONTROL) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT_ALT] = glfwGetKey(_window, GLFW_KEY_RIGHT_ALT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_RIGHT_SUPER] = glfwGetKey(_window, GLFW_KEY_RIGHT_SUPER) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_MENU] = glfwGetKey(_window, GLFW_KEY_MENU) == GLFW_PRESS;

    currentKeyboardState[INPUT_KEY::KEY_MOUSE_LEFT] = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;
    currentKeyboardState[INPUT_KEY::KEY_MOUSE_RIGHT] = glfwGetMouseButton(_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS;
}
