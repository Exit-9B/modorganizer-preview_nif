#include "TextureManager.h"
#include "PreviewNif.h"

#include <iplugingame.h>
#include <dataarchives.h>

#include <gli/gli.hpp>
#include <libbsarch.h>

#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>
#include <QVector4D>

#include <memory>

TextureManager::TextureManager(MOBase::IOrganizer* moInfo) : m_MOInfo{ moInfo }
{}

void TextureManager::cleanup()
{
    for (auto it = m_Textures.begin(); it != m_Textures.end();) {
        auto texture = it.value();
        m_Textures.erase(it++);

        if (texture) {
            texture->destroy();
            delete texture;
        }
    }

    if (m_ErrorTexture) {
        m_ErrorTexture->destroy();
        delete m_ErrorTexture;
        m_ErrorTexture = nullptr;
    }

    if (m_BlackTexture) {
        m_BlackTexture->destroy();
        delete m_ErrorTexture;
        m_BlackTexture = nullptr;
    }

    if (m_WhiteTexture) {
        m_WhiteTexture->destroy();
        delete m_ErrorTexture;
        m_WhiteTexture = nullptr;
    }

    if (m_FlatNormalTexture) {
        m_FlatNormalTexture->destroy();
        delete m_ErrorTexture;
        m_FlatNormalTexture = nullptr;
    }
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

    auto key = texturePath.toLower();

    auto cached = m_Textures.find(key);
    if (cached != m_Textures.end()) {
        return cached.value();
    }

    auto texture = loadTexture(texturePath);

    m_Textures[key] = texture;
    return texture;
}

QOpenGLTexture* TextureManager::getErrorTexture()
{
    if (!m_ErrorTexture) {
        m_ErrorTexture = makeSolidColor({ 1.0f, 0.0f, 1.0f, 1.0f });
    }

    return m_ErrorTexture;
}

QOpenGLTexture* TextureManager::getBlackTexture()
{
    if (!m_BlackTexture) {
        m_BlackTexture = makeSolidColor({0.0f, 0.0f, 0.0f, 1.0f });
    }

    return m_BlackTexture;
}

QOpenGLTexture* TextureManager::getWhiteTexture()
{
    if (!m_WhiteTexture) {
        m_WhiteTexture = makeSolidColor({1.0f, 1.0f, 1.0f, 1.0f });
    }

    return m_WhiteTexture;
}

QOpenGLTexture* TextureManager::getFlatNormalTexture()
{
    if (!m_FlatNormalTexture) {
        m_FlatNormalTexture = makeSolidColor({0.5f, 0.5f, 1.0f, 1.0f });
    }

    return m_FlatNormalTexture;
}

QOpenGLTexture* TextureManager::loadTexture(QString texturePath)
{
    if (texturePath.isEmpty()) {
        return nullptr;
    }

    auto game = m_MOInfo->managedGame();

    if (!game) {
        qCritical(qUtf8Printable(QObject::tr("Failed to interface with managed game plugin")));
        return nullptr;
    }

    auto realPath = resolvePath(game, texturePath);
    if (!realPath.isEmpty()) {
        return makeTexture(gli::load(realPath.toStdString()));
    }

    auto gameArchives = game->feature<DataArchives>();
    if (!gameArchives) {
        return nullptr;
    }

    auto archives = gameArchives->archives(m_MOInfo->profile());
    for (auto it = archives.rbegin(); it != archives.rend(); ++it) {
        auto& archive = *it;

        auto bsaPath = resolvePath(game, archive);
        if (bsaPath.isEmpty()) {
            continue;
        }

        using bsa_ptr = std::unique_ptr<void, decltype(&bsa_free)>;
        auto bsa = bsa_ptr(bsa_create(), bsa_free);

        static_assert(sizeof(wchar_t) == 2, "Expected wchar_t to be 2 bytes");

        bsa_result_message_t result;
        auto bsaPath_utf16 = reinterpret_cast<const wchar_t*>(bsaPath.utf16());
        result = bsa_load_from_file(bsa.get(), bsaPath_utf16);
        if (result.code == BSA_RESULT_EXCEPTION) {
            continue;
        }

        auto texturePath_utf16 = reinterpret_cast<const wchar_t*>(texturePath.utf16());
        auto result_buffer = bsa_extract_file_data_by_filename(bsa.get(), texturePath_utf16);
        if (result_buffer.message.code == BSA_RESULT_EXCEPTION) {
            continue;
        }

        auto buffer_free =
            [&bsa](bsa_result_buffer_t* buffer) {
                bsa_file_data_free(bsa.get(), *buffer);
            };
        using buffer_ptr = std::unique_ptr<bsa_result_buffer_t, decltype(buffer_free)>;
        auto buffer = buffer_ptr(&result_buffer.buffer, buffer_free);

        auto data = static_cast<char*>(buffer->data);
        if (auto texture = makeTexture(gli::load(data, buffer->size))) {
            return texture;
        }
    }

    return nullptr;
}

QOpenGLTexture* TextureManager::makeTexture(const gli::texture& texture)
{
    if (texture.empty()) {
        return nullptr;
    }

    gli::gl GL(gli::gl::PROFILE_GL32);
    const gli::gl::format format = GL.translate(texture.format(), texture.swizzles());
    GLenum target = GL.translate(texture.target());

    auto f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_1>();
    QOpenGLTexture* glTexture = new QOpenGLTexture(static_cast<QOpenGLTexture::Target>(target));

    glTexture->create();
    glTexture->bind();
    glTexture->setMipLevels(texture.levels());
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

    glTexture->setSize(extent.x, extent.y, extent.z);
    glTexture->setFormat(static_cast<QOpenGLTexture::TextureFormat>(format.Internal));
    glTexture->allocateStorage(
        static_cast<QOpenGLTexture::PixelFormat>(format.External),
        static_cast<QOpenGLTexture::PixelType>(format.Type));

    for (std::size_t layer = 0; layer < texture.layers(); layer++)
    for (std::size_t face = 0; face < texture.faces(); face++)
    for (std::size_t level = 0; level < texture.levels(); level++)
    {
        auto extent = texture.extent(level);

        // Qt's upload functions lag badly so we just use the GL API
        switch (texture.target()) {
        case gli::TARGET_1D:
            if (gli::is_compressed(texture.format())) {
                f->glCompressedTexSubImage1D(
                    glTexture->target(),
                    level,
                    0,
                    extent.x,
                    format.Internal,
                    texture.size(level),
                    texture.data(layer, face, level));
            }
            else {
                f->glTexSubImage1D(
                    glTexture->target(),
                    level,
                    0,
                    extent.x,
                    format.External,
                    format.Type,
                    texture.data(layer, face, level));
            }
            break;
        case gli::TARGET_1D_ARRAY:
        case gli::TARGET_2D:
        case gli::TARGET_CUBE:
            if (gli::is_compressed(texture.format())) {
                f->glCompressedTexSubImage2D(
                    glTexture->target(),
                    level,
                    0, 0,
                    extent.x,
                    texture.target() == gli::TARGET_1D_ARRAY ? layer : extent.y,
                    format.Internal,
                    texture.size(level),
                    texture.data(layer, face, level));
            }
            else {
                f->glTexSubImage2D(
                    glTexture->target(),
                    level,
                    0, 0,
                    extent.x,
                    texture.target() == gli::TARGET_1D_ARRAY ? layer : extent.y,
                    format.External,
                    format.Type,
                    texture.data(layer, face, level));
            }
            break;
        case gli::TARGET_2D_ARRAY:
        case gli::TARGET_3D:
        case gli::TARGET_CUBE_ARRAY:
            if (gli::is_compressed(texture.format())) {
                f->glCompressedTexSubImage3D(
                    glTexture->target(),
                    level,
                    0, 0, 0,
                    extent.x, extent.y,
                    texture.target() == gli::TARGET_3D ? extent.z : layer,
                    format.Internal,
                    texture.size(level),
                    texture.data(layer, face, level));
            }
            else {
                f->glTexSubImage3D(
                    glTexture->target(),
                    level,
                    0, 0, 0,
                    extent.x, extent.y,
                    texture.target() == gli::TARGET_3D ? extent.z : layer,
                    format.External,
                    format.Type,
                    texture.data(layer, face, level));
            }
            break;
        }
    }

    glTexture->release();

    return glTexture;
}

QOpenGLTexture* TextureManager::makeSolidColor(QVector4D color)
{
    QOpenGLTexture* glTexture = new QOpenGLTexture(QOpenGLTexture::Target2D);
    glTexture->create();
    glTexture->bind();

    glTexture->setSize(1, 1);
    glTexture->setFormat(QOpenGLTexture::RGBA32F);
    glTexture->allocateStorage(QOpenGLTexture::RGBA, QOpenGLTexture::Float32);

    glTexture->setData(QOpenGLTexture::RGBA, QOpenGLTexture::Float32, &color);

    glTexture->release();

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
