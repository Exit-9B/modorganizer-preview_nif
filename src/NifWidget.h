#pragma once

#include <NifFile.hpp>

#include <imoinfo.h>
#include <QOpenGLWidget>

#include <QOpenGLVertexArrayObject>
#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>

#include "OpenGLShape.h"
#include "TextureManager.h"

#include <memory>

class NifWidget : public QOpenGLWidget
{
    Q_OBJECT

public:
    NifWidget(
        std::shared_ptr<nifly::NifFile> nifFile,
        MOBase::IOrganizer* organizer,
        bool debugContext = false,
        QWidget* parent = nullptr,
        Qt::WindowFlags f = {0});

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void cleanup();
    void initProgram();
    void updateCamera();

    std::shared_ptr<nifly::NifFile> m_NifFile;
    MOBase::IOrganizer* m_MOInfo = nullptr;

    std::unique_ptr<TextureManager> m_TextureManager;

    QOpenGLDebugLogger* m_Logger = nullptr;

    QOpenGLShaderProgram* m_Program = nullptr;
    std::vector<OpenGLShape> m_GLShapes;

    QVector3D m_CameraLookAt;
    float m_Pitch = 0.0f;
    float m_Yaw = 0.0f;
    float m_CameraDistance = 100.0f;

    QMatrix4x4 m_ViewMatrix;
    QMatrix4x4 m_ProjectionMatrix;

    int m_ViewportWidth;
    int m_ViewportHeight;
    QPoint m_MousePos;
};
