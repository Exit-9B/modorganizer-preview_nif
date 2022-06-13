#pragma once

#include <imoinfo.h>
#include <gli/gli.hpp>
#include <QOpenGLTexture>

class TextureManager
{
public:
    TextureManager(MOBase::IOrganizer* organizer);
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    void initialize();
    QOpenGLTexture* getTexture(const std::string& texturePath);
    QOpenGLTexture* getTexture(QString texturePath);

private:
    QOpenGLTexture* makeTexture(const gli::texture& texture);
    QString resolvePath(const MOBase::IPluginGame* game, QString path);

    MOBase::IOrganizer* m_MOInfo;
    QOpenGLTexture* m_ErrorTexture = nullptr;
};
