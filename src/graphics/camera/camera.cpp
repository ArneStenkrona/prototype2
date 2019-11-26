#include "camera.h"

Camera::Camera(Input& input, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : _position(position), 
      _front(glm::vec3(0.0f, 0.0f, -1.0f)),
      _worldUp(up), 
      _yaw(yaw), 
      _pitch(pitch),
      _movementSpeed(2.5f), 
      _mouseSensitivity(0.30f), 
      _fieldOfView(45.0f),
      _input(input)
{
    updateCameraVectors();
}

Camera::Camera(Input& input, float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Camera(input, glm::vec3(posX, posY, posZ), glm::vec3(upX, upY, upZ), yaw, pitch)
{
}

glm::mat4 Camera::getProjectionMatrix() const
{
    /* TODO: Get screen dimensions instead of hardcoding 800x600  */
    return glm::perspective(glm::radians(_fieldOfView), 800.0f / 600.0f, 0.1f, 100.0f);
}

void Camera::update(float deltaTime)
{
    processKeyboard(deltaTime);
    processMouseMovement();
}

void Camera::processKeyboard(float deltaTime)
{
    //Movement
    float cameraSpeed = _movementSpeed * deltaTime;
    if (_input.getKey(GLFW_KEY_W) == GLFW_PRESS)
        _position += _front * cameraSpeed;
    if (_input.getKey(GLFW_KEY_S) == GLFW_PRESS)
        _position -= _front * cameraSpeed;
    if (_input.getKey(GLFW_KEY_A) == GLFW_PRESS)
        _position -= glm::normalize(glm::cross(_front, _up)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_D) == GLFW_PRESS)
        _position += glm::normalize(glm::cross(_front, _up)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_SPACE) == GLFW_PRESS)
        _position += glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        _position -= glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    //Reset field of view
    if (_input.getKey(GLFW_KEY_R) == GLFW_PRESS)
        _fieldOfView = 45.0f;
}

void Camera::processMouseMovement()
{
    //Mouse
    double x, y;
    _input.getCursorDelta(x, y);
    float dx = (float)x * _mouseSensitivity, dy = (float)y * _mouseSensitivity;

    _yaw += dx;
    _pitch -= dy;
    
    // constrainPitch should have its effect here
    if (_pitch > 89.0f)
        _pitch = 89.0f;
    if (_pitch < -89.0f)
        _pitch = -89.0f;

    glm::vec3 f;
    f.x = cos(glm::radians(_pitch)) * cos(glm::radians(_yaw));
    f.y = sin(glm::radians(_pitch));
    f.z = cos(glm::radians(_pitch)) * sin(glm::radians(_yaw));

    _front = glm::normalize(f);
    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 f;
    f.x = cos(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    f.y = sin(glm::radians(_pitch));
    f.z = sin(glm::radians(_yaw)) * cos(glm::radians(_pitch));
    _front = glm::normalize(f);
    // Also re-calculate the Right and Up vector
    _right = glm::normalize(glm::cross(_front, _worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    _up = glm::normalize(glm::cross(_right, _front));
}