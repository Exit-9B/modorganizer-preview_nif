#include "ShaderManager.h"

#include <QOpenGLContext>

ShaderManager::ShaderManager(MOBase::IOrganizer* moInfo) : m_MOInfo{ moInfo }
{}

QOpenGLShaderProgram* ShaderManager::getProgram(ShaderType type)
{
    if (m_Programs[type] == nullptr) {
        m_Programs[type] = loadProgram(type);
    }

    return m_Programs[type];
}

QOpenGLShaderProgram* ShaderManager::loadProgram(ShaderType type)
{
    QString name;

    switch (type) {
    case SkDefault:
        name = "sk_default";
        break;
    default:
        name = "sk_default";
        break;
    }

    auto game = m_MOInfo->managedGame();
    auto dataPath = MOBase::IOrganizer::getPluginDataPath();
    auto vertexShader = QString("%1/shaders/%2.vert").arg(dataPath).arg(name);
    auto fragmentShader = QString("%1/shaders/%2.frag").arg(dataPath).arg(name);

    auto program = new QOpenGLShaderProgram(QOpenGLContext::currentContext());
    program->addShaderFromSourceFile(QOpenGLShader::Vertex, vertexShader);
    program->addShaderFromSourceFile(QOpenGLShader::Fragment, fragmentShader);

    // Currently the same for every program
    program->bindAttributeLocation("position", 0);
    program->bindAttributeLocation("normal", 1);
    program->bindAttributeLocation("tangent", 2);
    program->bindAttributeLocation("bitangent", 3);
    program->bindAttributeLocation("texCoord", 4);
    program->bindAttributeLocation("color", 5);

    program->link();

    return program;
}
