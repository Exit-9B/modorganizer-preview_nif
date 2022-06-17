#include "Camera.h"

#include <cmath>

void Camera::setDistance(float distance)
{
    m_Distance = qBound(MinDistance, distance, MaxDistance);
    cameraMoved();
}

void Camera::setLookAt(QVector3D lookAt)
{
    m_LookAt = lookAt;
    cameraMoved();
}

void Camera::pan(QVector3D distance)
{
    m_LookAt += distance;
    cameraMoved();
}

void Camera::rotate(float yaw, float pitch)
{
    m_Yaw = repeat(m_Yaw + yaw, 0.0f, 360.0f);
    m_Pitch = repeat(m_Pitch + pitch, 0.0f, 360.0f);

    cameraMoved();
}

void Camera::zoomDistance(float distance)
{
    m_Distance += distance;
    m_Distance = qBound(MinDistance, m_Distance, MaxDistance);

    cameraMoved();
}

void Camera::zoomFactor(float factor)
{
    m_Distance *= factor;
    m_Distance = qBound(MinDistance, m_Distance, MaxDistance);

    cameraMoved();
}

float Camera::repeat(float value, float min, float max)
{
    return fmod(fmod(value, max - min) + (max - min), max - min) + min;
}
