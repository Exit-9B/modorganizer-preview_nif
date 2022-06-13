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
      m_TextureManager{ std::make_unique<TextureManager>(moInfo) }
{
    if (debugContext) {
        setFormat({ QSurfaceFormat::DebugContext });
        m_Logger = new QOpenGLDebugLogger(this);
    }
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
        m_Yaw += delta.x() * 0.5f;
        m_Pitch += delta.y() * 0.5f;
    } break;
    case Qt::MiddleButton:
    {
        float viewDX = m_CameraDistance / m_ViewportWidth;
        float viewDY = m_CameraDistance / m_ViewportHeight;

        QMatrix4x4 r;
        r.rotate(-m_Yaw, 0.0f, 1.0f, 0.0f);
        r.rotate(-m_Pitch, 1.0f, 0.0f, 0.0f);

        auto pan = r * QVector4D(-delta.x() * viewDX, delta.y() * viewDY, 0.0f, 0.0f);

        m_CameraLookAt += QVector3D(pan);
    } break;
    case Qt::RightButton:
    {
        if (event->modifiers() == Qt::ShiftModifier) {
            m_CameraDistance += delta.y() * 0.5f;
        }
    } break;
    }

    updateCamera();
    update();
}

void NifWidget::wheelEvent(QWheelEvent* event)
{
    m_CameraDistance *= 1.0f - (event->angleDelta().y() / 120.0f * 0.38f);

    updateCamera();
    update();
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

    auto f = QOpenGLContext::currentContext()->versionFunctions<OpenGLFunctions>();
    connect(
        QOpenGLContext::currentContext(),
        &QOpenGLContext::aboutToBeDestroyed,
        this,
        &NifWidget::cleanup);

    initProgram();

    auto shapes = m_NifFile->GetShapes();
    for (auto& shape : shapes) {
        m_GLShapes.emplace_back(m_NifFile.get(), shape, m_TextureManager.get());
    }

    float largestRadius = 0.0f;
    for (auto& shape : shapes) {
        if (auto vertices = m_NifFile->GetVertsForShape(shape)) {
            auto bounds = nifly::BoundingSphere(*vertices);

            if (bounds.radius > largestRadius) {
                largestRadius = bounds.radius;

                m_CameraDistance = bounds.radius * 2.4f;
                m_CameraLookAt = { -bounds.center.x, bounds.center.z, bounds.center.y };
            }
        }
    }

    updateCamera();

    f->glEnable(GL_DEPTH_TEST);
    f->glEnable(GL_ALPHA_TEST);
    f->glEnable(GL_CULL_FACE);
    f->glClearColor(0.18, 0.18, 0.18, 1.0);
}

void NifWidget::paintGL()
{
    auto f = QOpenGLContext::currentContext()->versionFunctions<OpenGLFunctions>();
    f->glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (m_Program->bind()) {
        for (auto& shape : m_GLShapes) {
            auto binder = QOpenGLVertexArrayObject::Binder(shape.vertexArray);

            auto modelMatrix = shape.modelMatrix;
            auto modelViewMatrix = m_ViewMatrix * modelMatrix;
            auto mvpMatrix = m_ProjectionMatrix * modelViewMatrix;

            m_Program->setUniformValue("modelViewMatrix", modelViewMatrix);
            m_Program->setUniformValue("normalMatrix", modelViewMatrix.normalMatrix());
            m_Program->setUniformValue("mvpMatrix", mvpMatrix);
            m_Program->setUniformValue("worldMatrix", QMatrix4x4());
            m_Program->setUniformValue("lightDirection", QVector3D(0, 0, 1));

            shape.setupShaders(m_Program);

            if (shape.indexBuffer && shape.indexBuffer->isCreated()) {
                shape.indexBuffer->bind();
                f->glDrawElements(GL_TRIANGLES, shape.elements, GL_UNSIGNED_SHORT, nullptr);
                shape.indexBuffer->release();
            }
        }
        m_Program->release();
    }
}

void NifWidget::resizeGL(int w, int h)
{
    auto f = QOpenGLContext::currentContext()->versionFunctions<OpenGLFunctions>();

    f->glViewport(0, 0, w, h);

    QMatrix4x4 m;
    m.perspective(40.0f, static_cast<float>(w) / h, 0.1f, 10000.0f);

    m_ProjectionMatrix = m;
    m_ViewportWidth = w;
    m_ViewportHeight = h;
}

void NifWidget::cleanup()
{
    for (auto& shape : m_GLShapes) {
        shape.destroy();
    }
    m_GLShapes.clear();
}

void NifWidget::initProgram()
{
    auto game = m_MOInfo->managedGame();
    auto dataPath = MOBase::IOrganizer::getPluginDataPath();
    auto vertexShader = dataPath + "/shaders/sk_default.vert";
    auto fragmentShader = dataPath + "/shaders/sk_default.frag";

    m_Program = new QOpenGLShaderProgram(this);
    m_Program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShader);
    m_Program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShader);
    m_Program->bindAttributeLocation("position", 0);
    m_Program->bindAttributeLocation("normal", 1);
    m_Program->bindAttributeLocation("tangent", 2);
    m_Program->bindAttributeLocation("bitangent", 3);
    m_Program->bindAttributeLocation("texCoord", 4);
    m_Program->bindAttributeLocation("color", 5);
    m_Program->link();
}

void NifWidget::updateCamera()
{
    m_CameraDistance = qBound(1.0f, m_CameraDistance, 1000.0f);

    QMatrix4x4 m;
    m.translate(0.0f, 0.0f, -m_CameraDistance);
    m.rotate(m_Pitch, 1.0f, 0.0f, 0.0f);
    m.rotate(m_Yaw, 0.0f, 1.0f, 0.0f);
    m.translate(-m_CameraLookAt);
    m *= QMatrix4x4{
        -1, 0, 0, 0,
         0, 0, 1, 0,
         0, 1, 0, 0,
         0, 0, 0, 1
    };
    m_ViewMatrix = m;
}
