#pragma once

#include <QObject>
#include <QVector3D>

class Camera : public QObject
{
    Q_OBJECT

public:
    QVector3D lookAt() { return m_LookAt; }
    float pitch() { return m_Pitch; }
    float yaw() { return m_Yaw; }
    float distance() { return m_Distance; }

    void setDistance(float distance);
    void setLookAt(QVector3D lookAt);

    void pan(QVector3D delta);
    void rotate(float yaw, float pitch);
    void zoomDistance(float distance);
    void zoomFactor(float factor);

private:
    inline static constexpr float MinDistance = 1.0f;
    inline static constexpr float MaxDistance = 5000.0f;

    static float repeat(float value, float min, float max);

    QVector3D m_LookAt;
    float m_Pitch = 0.0f;
    float m_Yaw = 0.0f;
    float m_Distance = 100.0f;

signals:
    void cameraMoved();
};
