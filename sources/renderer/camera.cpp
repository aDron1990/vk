#include "renderer/camera.hpp"

#include <glm/gtc/matrix_transform.hpp>

Camera::Camera()
{
    m_position = { 0.0f, 0.0f, 4.0f };
    updateCamera();
}

void Camera::updateCamera()
{
    glm::vec3 front;
    front.x = cos(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    front.y = sin(glm::radians(m_pitch));
    front.z = sin(glm::radians(m_yaw)) * cos(glm::radians(m_pitch));
    m_front = glm::normalize(front);
    m_right = glm::normalize(glm::cross(m_front, { 0.0f, 1.0f, 0.0f }));
    m_up = glm::normalize(glm::cross(m_right, m_front));
}

void Camera::move(glm::vec3 direction)
{
    m_position += m_front * direction.z * SPEED;
    m_position += m_right * direction.x * SPEED;
    m_position += glm::vec3{ 0.0f, 1.0f, 0.0f } * direction.y * SPEED;
    updateCamera();
}

void Camera::rotate(glm::vec2 rotation)
{
    m_yaw += rotation.x * RSPEED;
    m_pitch -= rotation.y * RSPEED;
    if (m_pitch > 89.0f) m_pitch = 89.0f;
    if (m_pitch < -89.0f) m_pitch = -89.0f;
    updateCamera();
}

glm::mat4 Camera::getViewMatrix()
{
    return glm::lookAt(m_position, m_position + m_front, m_up);
}