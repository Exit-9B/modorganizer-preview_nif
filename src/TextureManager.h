#pragma once

#include <imoinfo.h>
#include <gli/gli.hpp>
#include <QOpenGLTexture>
#include <map>

class TextureManager
{
public:
    TextureManager(MOBase::IOrganizer* organizer);
    ~TextureManager() = default;
    TextureManager(const TextureManager&) = delete;
    TextureManager(TextureManager&&) = delete;
    TextureManager& operator=(const TextureManager&) = delete;
    TextureManager& operator=(TextureManager&&) = delete;

    void cleanup();

    QOpenGLTexture* getTexture(const std::string& texturePath);
    QOpenGLTexture* getTexture(QString texturePath);

    QOpenGLTexture* getErrorTexture();
    QOpenGLTexture* getBlackTexture();
    QOpenGLTexture* getWhiteTexture();
    QOpenGLTexture* getFlatNormalTexture();

private:
    QOpenGLTexture* loadTexture(QString texturePath);
    QOpenGLTexture* makeTexture(const gli::texture& texture);
    QOpenGLTexture* makeSolidColor(QVector4D color);

    QString resolvePath(const MOBase::IPluginGame* game, QString path);

    MOBase::IOrganizer* m_MOInfo;
    QOpenGLTexture* m_ErrorTexture = nullptr;
    QOpenGLTexture* m_BlackTexture = nullptr;
    QOpenGLTexture* m_WhiteTexture = nullptr;
    QOpenGLTexture* m_FlatNormalTexture = nullptr;

    std::map<std::wstring, QOpenGLTexture*> m_Textures;
};
