#include "camera.h"

Camera::Camera(Input& input, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : m_position(position), 
      m_front(glm::vec3(0.0f, 0.0f, -1.0f)),
      m_worldUp(up), 
      m_yaw(yaw), 
      m_pitch(pitch),
      m_movementSpeed(25.0f), 
      m_mouseSensitivity(0.30f), 
      m_fieldOfView(45.0f),
      m_width(800.0f),
      m_height(600.0f),
      m_nearPlane(0.3f),
      m_farPlane(100.0f),
      m_targetDistance(5.0f),
      m_input(input) {
    updateCameraVectors();
}

Camera::Camera(Input& input, float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Camera(input, glm::vec3(posX, posY, posZ), glm::vec3(upX, upY, upZ), yaw, pitch) {
}

void Camera::setProjection(float width, float height, float near, float far) {
    assert(width > 0 && height > 0);
    m_width = width;
    m_height = height;
    m_nearPlane = near;
    m_farPlane = far;
}

glm::mat4 Camera::getProjectionMatrix() const {
    return getProjectionMatrix(m_nearPlane, m_farPlane);
}

glm::mat4 Camera::getProjectionMatrix(float near, float far) const {
    return glm::perspective(glm::radians(m_fieldOfView), m_width / m_height, near, far);
}

void Camera::getCameraCorners(glm::vec3 & topleft,
                              glm::vec3 & topright,
                              glm::vec3 & bottomleft,
                              glm::vec3 & bottomright) {
    glm::mat4 invmvp = glm::inverse(getProjectionMatrix() * getViewMatrix());
    glm::vec4 tl     = invmvp * glm::vec4{ -1.0f, -1.0f, -1.0f, 1.0f };
    glm::vec4 tr    = invmvp * glm::vec4{ 1.0f, -1.0f, -1.0f, 1.0f };
    glm::vec4 bl  = invmvp * glm::vec4{ -1.0f, 1.0f, -1.0f, 1.0f };
    glm::vec4 br = invmvp * glm::vec4{ 1.0f, 1.0f, -1.0f, 1.0f };
    topleft = tl / tl.w;
    topright = tr / tl.w;
    bottomleft = bl / tl.w;
    bottomright = br / tl.w;
}

void Camera::rotate(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
}

void Camera::setTarget(glm::vec3 target) {
    m_position = target - (m_targetDistance * m_front);
    m_lastTargetPosition = target;
}

void Camera::update(float deltaTime, bool keyboardMovement, bool drag) {
    if (keyboardMovement) processKeyboard(deltaTime);
    processMouseMovement(drag);
}

void Camera::processKeyboard(float deltaTime) {
    //Movement
    float cameraSpeed = m_movementSpeed * deltaTime;
    if (m_input.getKeyPress(INPUT_KEY::KEY_W))
        m_position += m_front * cameraSpeed;
    if (m_input.getKeyPress(INPUT_KEY::KEY_S))
        m_position -= m_front * cameraSpeed;
    if (m_input.getKeyPress(INPUT_KEY::KEY_A))
        m_position -= glm::normalize(glm::cross(m_front, m_up)) * cameraSpeed;
    if (m_input.getKeyPress(INPUT_KEY::KEY_D))
        m_position += glm::normalize(glm::cross(m_front, m_up)) * cameraSpeed;
    if (m_input.getKeyPress(INPUT_KEY::KEY_SPACE))
        m_position += glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    if (m_input.getKeyPress(INPUT_KEY::KEY_LEFT_CONTROL))
        m_position -= glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    //Reset field of view
    if (m_input.getKeyPress(INPUT_KEY::KEY_R))
        m_fieldOfView = 45.0f;
}

void Camera::processMouseMovement(bool drag) {
    if (drag && !m_input.getKeyPress(INPUT_KEY::KEY_MOUSE_LEFT)) return;

    //Mouse
    double x, y;
    m_input.getCursorDelta(x, y);
    float dx = (float)x * m_mouseSensitivity, dy = (float)y * m_mouseSensitivity;

    m_yaw += dx;
    m_pitch -= dy;
    
    // constrainPitch should have its effect here
    if (m_pitch > 89.0f)
        m_pitch = 89.0f;
    if (m_pitch < -89.0f)
        m_pitch = -89.0f;

    glm::vec3 f;
    f.x = cos(glm::radians(m_pitch)) * cos(glm::radians(m_yaw));
    f.y = sin(glm::radians(m_pitch));
    f.z = cos(glm::radians(m_pitch)) * sin(glm::radians(m_yaw));

    m_front = glm::normalize(f);
    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::updateCameraVectors() {
    // Calculate the new Front vector
    glm::vec3 f;
    f.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    f.y = sin(glm::radians(m_pitch));
    f.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(f);
    // Also re-calculate the Right and Up vector
    m_right = glm::normalize(glm::cross(m_front, m_worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    m_up = glm::normalize(glm::cross(m_right, m_front));
}