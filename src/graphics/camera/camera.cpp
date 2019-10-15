#include "camera.h"

Camera::Camera(Input& input, glm::vec3 position, glm::vec3 up, float yaw, float pitch)
    : position(position), 
      front(glm::vec3(0.0f, 0.0f, -1.0f)),
      worldUp(up), 
      Yaw(yaw), 
      Pitch(pitch),
      MovementSpeed(2.5f), 
      MouseSensitivity(0.15f), 
      fieldOfView(45.0f),
      _input(input)
{
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    updateCameraVectors();
}

Camera::Camera(Input& input, float posX, float posY, float posZ, float upX, float upY, float upZ, float yaw, float pitch)
    : Camera(input, glm::vec3(posX, posY, posZ), glm::vec3(upX, upY, upZ), yaw, pitch)
{
}

glm::mat4 Camera::getProjectionMatrix() const
{
    /* TODO: Get screen dimensions instead of hardcoding 800x600  */
    return glm::perspective(glm::radians(fieldOfView), 800.0f / 600.0f, 0.1f, 100.0f);
}

void Camera::update(float deltaTime)
{
    processKeyboard(deltaTime);
    processMouseMovement();
}

void Camera::processKeyboard(float deltaTime)
{
    //Movement
    float cameraSpeed = MovementSpeed * deltaTime;
    if (_input.getKey(GLFW_KEY_W) == GLFW_PRESS)
        position += front * cameraSpeed;
    if (_input.getKey(GLFW_KEY_S) == GLFW_PRESS)
        position -= front * cameraSpeed;
    if (_input.getKey(GLFW_KEY_A) == GLFW_PRESS)
        position -= glm::normalize(glm::cross(front, up)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_D) == GLFW_PRESS)
        position += glm::normalize(glm::cross(front, up)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_SPACE) == GLFW_PRESS)
        position += glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    if (_input.getKey(GLFW_KEY_LEFT_CONTROL) == GLFW_PRESS)
        position -= glm::normalize(glm::vec3(0.0f, 1.0f, 0.0f)) * cameraSpeed;
    //Reset field of view
    if (_input.getKey(GLFW_KEY_R) == GLFW_PRESS)
        fieldOfView = 45.0f;
}

void Camera::processMouseMovement()
{
    //Mouse
    double x, y;
    _input.getCursorDelta(x, y);
    float dx = (float)x * MouseSensitivity, dy = (float)y * MouseSensitivity;

    Yaw += dx;
    Pitch -= dy;
    
    // constrainPitch should have its effect here
    if (Pitch > 89.0f)
        Pitch = 89.0f;
    if (Pitch < -89.0f)
        Pitch = -89.0f;

    glm::vec3 f;
    f.x = cos(glm::radians(Pitch)) * cos(glm::radians(Yaw));
    f.y = sin(glm::radians(Pitch));
    f.z = cos(glm::radians(Pitch)) * sin(glm::radians(Yaw));

    front = glm::normalize(f);
    // Update Front, Right and Up Vectors using the updated Euler angles
    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    // Calculate the new Front vector
    glm::vec3 f;
    f.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    f.y = sin(glm::radians(Pitch));
    f.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
    front = glm::normalize(f);
    // Also re-calculate the Right and Up vector
    right = glm::normalize(glm::cross(front, worldUp));  // Normalize the vectors, because their length gets closer to 0 the more you look up or down which results in slower movement.
    up = glm::normalize(glm::cross(right, front));
}