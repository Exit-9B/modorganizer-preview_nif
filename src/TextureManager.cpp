#include "TextureManager.h"
#include "PreviewNif.h"

#include <iplugingame.h>
#include <dataarchives.h>

#include <gli/gli.hpp>
#include <libbsarch.h>
#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

TextureManager::TextureManager(MOBase::IOrganizer* moInfo) : m_MOInfo{ moInfo }
{}

void TextureManager::initialize()
{
    // TODO: Load default textures
}

QOpenGLTexture* TextureManager::getTexture(const std::string& texturePath)
{
    return getTexture(QString::fromStdString(texturePath));
}

QOpenGLTexture* TextureManager::getTexture(QString texturePath)
{
    if (texturePath.isEmpty()) {
        return nullptr;
    }

    auto game = m_MOInfo->managedGame();

    if (!game) {
        qCritical(qUtf8Printable(QObject::tr("Failed to interface with managed game plugin")));
        return m_ErrorTexture;
    }

    auto realPath = resolvePath(game, texturePath);
    if (!realPath.isEmpty()) {
        return makeTexture(gli::load(realPath.toStdString()));
    }

    auto gameArchives = game->feature<DataArchives>();
    if (!gameArchives) {
        return m_ErrorTexture;
    }

    QOpenGLTexture* texture = m_ErrorTexture;
    auto archives = gameArchives->archives(m_MOInfo->profile());
    for (auto it = archives.rbegin(); it != archives.rend(); ++it) {
        auto& archive = *it;

        auto bsaPath = resolvePath(game, archive);
        if (bsaPath.isEmpty()) {
            continue;
        }

        auto bsa = bsa_create();

        bsa_result_message_t result;
        static_assert(sizeof(wchar_t) == 2, "Expected wchar_t to be 2 bytes");
        auto bsaPath_utf16 = reinterpret_cast<const wchar_t*>(bsaPath.utf16());
        result = bsa_load_from_file(bsa, bsaPath_utf16);
        if (result.code == BSA_RESULT_EXCEPTION) {
            bsa_free(bsa);
            continue;
        }

        auto texturePath_utf16 = reinterpret_cast<const wchar_t*>(texturePath.utf16());
        auto result_buffer = bsa_extract_file_data_by_filename(bsa, texturePath_utf16);
        if (result_buffer.message.code == BSA_RESULT_EXCEPTION) {
            bsa_free(bsa);
            continue;
        }

        auto& buffer = result_buffer.buffer;
        texture = makeTexture(gli::load(static_cast<char*>(buffer.data), buffer.size));

        bsa_file_data_free(bsa, buffer);
        bsa_free(bsa);
    }

    return texture;
}

QOpenGLTexture* TextureManager::makeTexture(const gli::texture& texture)
{
    if (texture.empty()) {
        return nullptr;
    }

    gli::gl GL(gli::gl::PROFILE_ES20);
    const gli::gl::format format = GL.translate(texture.format(), texture.swizzles());
    GLenum target = GL.translate(texture.target());

    QOpenGLTexture* glTexture = new QOpenGLTexture(static_cast<QOpenGLTexture::Target>(target));
    glTexture->create();
    glTexture->bind();
    glTexture->setMipBaseLevel(0);
    glTexture->setMipMaxLevel(texture.levels() - 1);
    glTexture->setMinMagFilters(QOpenGLTexture::LinearMipMapLinear, QOpenGLTexture::Linear);
    glTexture->setSwizzleMask(
        static_cast<QOpenGLTexture::SwizzleValue>(format.Swizzles[0]),
        static_cast<QOpenGLTexture::SwizzleValue>(format.Swizzles[1]),
        static_cast<QOpenGLTexture::SwizzleValue>(format.Swizzles[2]),
        static_cast<QOpenGLTexture::SwizzleValue>(format.Swizzles[3]));

    auto extent = texture.extent();
    const GLsizei faceTotal = texture.layers() * texture.faces();

    switch (texture.target()) {
    case gli::TARGET_2D:
    case gli::TARGET_CUBE:
        glTexture->setSize(extent.x, extent.y);
        glTexture->setFormat(static_cast<QOpenGLTexture::TextureFormat>(format.Internal));
        glTexture->allocateStorage(
            static_cast<QOpenGLTexture::PixelFormat>(format.External),
            static_cast<QOpenGLTexture::PixelType>(format.Type));
        break;
    default:
        qWarning(qUtf8Printable(QObject::tr("Encountered unsupported texture target")));
        return nullptr;
    }

    for (std::size_t layer = 0; layer < texture.layers(); layer++)
    for (std::size_t level = 0; level < texture.levels(); level++)
    for (std::size_t face = 0; face < texture.faces(); face++)
    {
        auto extent = texture.extent(level);

        switch (texture.target()) {
        case gli::TARGET_2D:
            if (gli::is_compressed(texture.format())) {
                glTexture->setCompressedData(
                    level,
                    layer,
                    texture.size(level),
                    texture.data(layer, face, level));
            }
            else {
                glTexture->setData(
                    level,
                    layer,
                    static_cast<QOpenGLTexture::PixelFormat>(format.External),
                    static_cast<QOpenGLTexture::PixelType>(format.Type),
                    texture.data(layer, face, level));
            }
            break;
        case gli::TARGET_CUBE:
            if (gli::is_compressed(texture.format())) {
                glTexture->setCompressedData(level, layer,
                    static_cast<QOpenGLTexture::CubeMapFace>(
                        QOpenGLTexture::CubeMapPositiveX + face),
                    texture.size(level),
                    texture.data(layer, face, level));
            }
            else {
                glTexture->setData(
                    level,
                    layer,
                    static_cast<QOpenGLTexture::CubeMapFace>(
                        QOpenGLTexture::CubeMapPositiveX + face),
                    static_cast<QOpenGLTexture::PixelFormat>(format.External),
                    static_cast<QOpenGLTexture::PixelType>(format.Type),
                    texture.data(layer, face, level));
            }
            break;
        default:
            return nullptr;
        }
    }

    return glTexture;
}

QString TextureManager::resolvePath(const MOBase::IPluginGame* game, QString path)
{
    auto dataDir = game->dataDirectory();

    auto realPath = m_MOInfo->resolvePath(path);
    if (!realPath.isEmpty()) {
        return realPath;
    }

    auto dataPath = dataDir.absoluteFilePath(QDir::cleanPath(path));
    dataPath.replace('/', QDir::separator());

    if (QFileInfo::exists(dataPath)) {
        return dataPath;
    }

    return "";
}
