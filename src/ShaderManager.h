#pragma once

#include <imoinfo.h>
#include <QOpenGLShaderProgram>

class ShaderManager
{
public:
    enum ShaderType
    {
        SkDefault,
        SkMsn,
        SkEffectShader,
        Fo4Default,
        Fo4EffectShader,

        SHADER_COUNT,
    };

    ShaderManager(MOBase::IOrganizer* moInfo);
    ~ShaderManager() = default;
    ShaderManager(const ShaderManager&) = delete;
    ShaderManager(ShaderManager&&) = delete;
    ShaderManager& operator=(const ShaderManager&) = delete;
    ShaderManager& operator=(ShaderManager&&) = delete;

    QOpenGLShaderProgram* getProgram(ShaderType type);

private:
    QOpenGLShaderProgram* loadProgram(ShaderType type);

    MOBase::IOrganizer* m_MOInfo;
    QOpenGLShaderProgram* m_Programs[SHADER_COUNT] { nullptr };
};
