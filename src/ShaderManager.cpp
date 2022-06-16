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
    QString vert;
    QString frag;

    switch (type) {
    case SKDefault:
        vert = "sk_default.vert";
        frag = "sk_default.frag";
        break;
    case SKMSN:
        vert = "sk_msn.vert";
        frag = "sk_msn.frag";
        break;
    case SKMultilayer:
        vert = "sk_default.vert";
        frag = "sk_multilayer.frag";
        break;
    case SKEffectShader:
        vert = "sk_effectshader.vert";
        frag = "sk_multilayer.frag";
        break;
    case FO4Default:
        vert = "fo4_default.vert";
        frag = "fo4_default.frag";
        break;
    case FO4EffectShader:
        vert = "fo4_effectshader.vert";
        frag = "fo4_effectshader.frag";
        break;
    default:
        return nullptr;
    }

    auto game = m_MOInfo->managedGame();
    auto dataPath = MOBase::IOrganizer::getPluginDataPath();
    auto vertexShader = QString("%1/shaders/%2").arg(dataPath).arg(vert);
    auto fragmentShader = QString("%1/shaders/%2").arg(dataPath).arg(frag);

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
