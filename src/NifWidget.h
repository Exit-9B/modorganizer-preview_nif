#pragma once

#include "Camera.h"
#include "OpenGLShape.h"
#include "ShaderManager.h"
#include "TextureManager.h"

#include <QOpenGLBuffer>
#include <QOpenGLDebugLogger>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>
#include <QOpenGLWidget>
#include <QSharedPointer>

#include <imoinfo.h>
#include <NifFile.hpp>

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

    ~NifWidget();
    NifWidget(const NifWidget&) = delete;
    NifWidget(NifWidget&&) = delete;
    NifWidget& operator=(const NifWidget&) = delete;
    NifWidget& operator=(NifWidget&&) = delete;

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    void initializeGL() override;
    void paintGL() override;
    void resizeGL(int w, int h) override;

private:
    void cleanup();
    void updateCamera();

    inline static QWeakPointer<Camera> SharedCamera;

    std::shared_ptr<nifly::NifFile> m_NifFile;
    MOBase::IOrganizer* m_MOInfo = nullptr;

    std::unique_ptr<TextureManager> m_TextureManager;
    std::unique_ptr<ShaderManager> m_ShaderManager;

    QOpenGLDebugLogger* m_Logger = nullptr;

    std::vector<OpenGLShape> m_GLShapes;

    QSharedPointer<Camera> m_Camera;

    QMatrix4x4 m_ViewMatrix;
    QMatrix4x4 m_ProjectionMatrix;

    int m_ViewportWidth;
    int m_ViewportHeight;
    QPoint m_MousePos;
};
