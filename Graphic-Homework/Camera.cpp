#include "Camera.h"

Camera::Camera()
    : m_Position(0.0f, 0.0f, 3.0f),  // 預設位置
    m_Target(0.0f, 0.0f, 0.0f),      // 預設看向原點
    m_Up(0.0f, 1.0f, 0.0f),          // 預設 Y 軸向上
    m_Fov(45.0f),                    // 預設 45 度視野
    m_Near(0.1f),
    m_Far(100.0f)
{
}

void Camera::setPosition(const glm::vec3& pos)
{
    m_Position = pos;
}

void Camera::setTarget(const glm::vec3& target)
{
    m_Target = target;
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(m_Position, m_Target, m_Up);
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const
{
    return glm::perspective(glm::radians(m_Fov), aspectRatio, m_Near, m_Far);
}