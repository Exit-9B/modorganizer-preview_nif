#include "NifWidget.h"

#include <QMouseEvent>
#include <QWheelEvent>
#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>
using OpenGLFunctions = QOpenGLFunctions_2_1;

NifWidget::NifWidget(
    std::shared_ptr<nifly::NifFile> nifFile,
    MOBase::IOrganizer* moInfo,
    bool debugContext,
    QWidget* parent,
    Qt::WindowFlags f)
    : QOpenGLWidget(parent, f),
      m_NifFile{ nifFile },
      m_MOInfo{ moInfo },
      m_TextureManager{ std::make_unique<TextureManager>(moInfo) },
      m_ShaderManager{ std::make_unique<ShaderManager>(moInfo) }
{
    QSurfaceFormat format;
    format.setVersion(2, 1);
    format.setProfile(QSurfaceFormat::CoreProfile);

    if (debugContext) {
        format.setOption(QSurfaceFormat::DebugContext);
        m_Logger = new QOpenGLDebugLogger(this);
    }

    setFormat(format);
}

void NifWidget::mousePressEvent(QMouseEvent* event)
{
    m_MousePos = event->globalPos();
}

void NifWidget::mouseMoveEvent(QMouseEvent* event)
{
    auto pos = event->globalPos();
    auto delta = pos - m_MousePos;
    m_MousePos = pos;

    switch (event->buttons()) {
    case Qt::LeftButton:
    {
        m_Camera->rotate(delta.x() * 0.5, delta.y() * 0.5);
    } break;
    case Qt::MiddleButton:
    {
        float viewDX = m_Camera->distance() / m_ViewportWidth;
        float viewDY = m_Camera->distance() / m_ViewportHeight;

        QMatrix4x4 r;
        r.rotate(-m_Camera->yaw(), 0.0f, 1.0f, 0.0f);
        r.rotate(-m_Camera->pitch(), 1.0f, 0.0f, 0.0f);

        auto pan = r * QVector4D(-delta.x() * viewDX, delta.y() * viewDY, 0.0f, 0.0f);

        m_Camera->pan(QVector3D(pan));
    } break;
    case Qt::RightButton:
    {
        if (event->modifiers() == Qt::ShiftModifier) {
            m_Camera->zoomDistance(delta.y() * 0.1f);
        }
    } break;
    }
}

void NifWidget::wheelEvent(QWheelEvent* event)
{
    m_Camera->zoomFactor(1.0f - (event->angleDelta().y() / 120.0f * 0.38f));
}

void NifWidget::initializeGL()
{
    if (m_Logger) {
        m_Logger->initialize();
        connect(
            m_Logger,
            &QOpenGLDebugLogger::messageLogged,
            this,
            [](const QOpenGLDebugMessage& debugMessage){
                auto msg = tr("OpenGL debug message: %1").arg(debugMessage.message());
                qDebug(qUtf8Printable(msg));
            });
    }

    connect(
        context(),
        &QOpenGLContext::aboutToBeDestroyed,
        this,
        &NifWidget::cleanup,
        Qt::DirectConnection);

    auto shapes = m_NifFile->GetShapes();
    for (auto& shape : shapes) {
        m_GLShapes.emplace_back(m_NifFile.get(), shape, m_TextureManager.get());
    }

    m_Camera = SharedCamera;
    if (m_Camera.isNull()) {
        m_Camera = { new Camera(), &Camera::deleteLater };
        SharedCamera = m_Camera;

        float largestRadius = 0.0f;
        for (auto& shape : shapes) {
            if (auto vertices = m_NifFile->GetVertsForShape(shape)) {
                auto bounds = nifly::BoundingSphere(*vertices);

                if (bounds.radius > largestRadius) {
                    largestRadius = bounds.radius;

                    m_Camera->setDistance(bounds.radius * 2.4f);
                    m_Camera->setLookAt({ -bounds.center.x, bounds.center.z, bounds.center.y });
                }
            }
        }
    }

    updateCamera();

    connect(
        m_Camera.get(),
        &Camera::cameraMoved,
        this,
        [this](){
            updateCamera();
            update();
        });

    auto f = QOpenGLContext::currentContext()->versionFunctions<OpenGLFunctions>();

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_ALPHA_TEST);
    f->glEnable(GL_CULL_FACE);
    f->glClearColor(0.18, 0.18, 0.18, 1.0);
}

void NifWidget::paintGL()
{
    auto f = QOpenGLContext::currentContext()->versionFunctions<OpenGLFunctions>();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    for (auto& shape : m_GLShapes) {
        auto program = m_ShaderManager->getProgram(shape.shaderType);
        if (program && program->isLinked() && program->bind()) {
            auto binder = QOpenGLVertexArrayObject::Binder(shape.vertexArray);

            auto& modelMatrix = shape.modelMatrix;
            auto modelViewMatrix = m_ViewMatrix * modelMatrix;
            auto mvpMatrix = m_ProjectionMatrix * modelViewMatrix;

            program->setUniformValue("worldMatrix", modelMatrix);
            program->setUniformValue("viewMatrix", m_ViewMatrix);
            program->setUniformValue("modelViewMatrix", modelViewMatrix);
            program->setUniformValue("modelViewMatrixInverse", modelViewMatrix.inverted());
            program->setUniformValue("normalMatrix", modelViewMatrix.normalMatrix());
            program->setUniformValue("mvpMatrix", mvpMatrix);
            program->setUniformValue("lightDirection", QVector3D(0, 0, 1));

            shape.setupShaders(program);

            if (shape.indexBuffer && shape.indexBuffer->isCreated()) {
                shape.indexBuffer->bind();
                f->glDrawElements(GL_TRIANGLES, shape.elements, GL_UNSIGNED_SHORT, nullptr);
                shape.indexBuffer->release();
            }

            program->release();
        }
    }
}

void NifWidget::resizeGL(int w, int h)
{
    QMatrix4x4 m;
    m.perspective(40.0f, static_cast<float>(w) / h, 0.1f, 10000.0f);

    m_ProjectionMatrix = m;
    m_ViewportWidth = w;
    m_ViewportHeight = h;
}

void NifWidget::cleanup()
{
    makeCurrent();

    for (auto& shape : m_GLShapes) {
        shape.destroy();
    }
    m_GLShapes.clear();

    m_TextureManager->cleanup();
}

void NifWidget::updateCamera()
{
    QMatrix4x4 m;
    m.translate(0.0f, 0.0f, -m_Camera->distance());
    m.rotate(m_Camera->pitch(), 1.0f, 0.0f, 0.0f);
    m.rotate(m_Camera->yaw(), 0.0f, 1.0f, 0.0f);
    m.translate(-m_Camera->lookAt());
    m *= QMatrix4x4{
        -1, 0, 0, 0,
         0, 0, 1, 0,
         0, 1, 0, 0,
         0, 0, 0, 1,
    };
    m_ViewMatrix = m;
}
