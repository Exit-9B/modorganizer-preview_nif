#include "OpenGLShape.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

OpenGLShape::OpenGLShape(
    nifly::NifFile* nifFile,
    nifly::NiShape* niShape,
    TextureManager* textureManager)
{
    auto f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_1>();

    auto shader = nifFile->GetShader(niShape);
    auto& version = nifFile->GetHeader().GetVersion();
    if (version.IsFO4()) {
        if (shader->GetBlockName() == "BSEffectShaderProperty") {
            shaderType = ShaderManager::FO4EffectShader;
        }
        else {
            shaderType = ShaderManager::FO4Default;
        }
    }
    else {
        if (shader->GetBlockName() == "BSEffectShaderProperty") {
            shaderType = ShaderManager::SKEffectShader;
        }
        else {
            if (shader->IsModelSpace()) {
                shaderType = ShaderManager::SKMSN;
            }
            else if (shader->bslspShaderType == nifly::BSLSP_MULTILAYERPARALLAX) {
                shaderType = ShaderManager::SKMultilayer;
            }
            else {
                shaderType = ShaderManager::SKDefault;
            }
        }
    }

    vertexArray = new QOpenGLVertexArrayObject();
    vertexArray->create();
    auto binder = QOpenGLVertexArrayObject::Binder(vertexArray);

    auto numVertices = niShape->GetNumVertices();

    nifly::MatTransform transform;
    nifFile->GetNodeTransformToGlobal(niShape->name.get(), transform);
    modelMatrix = convertTransform(transform);

    if (niShape->HasVertices()) {
        vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (vertexBuffer->create()) {
            vertexBuffer->bind();

            if (auto vertices = nifFile->GetVertsForShape(niShape)) {
                vertexBuffer->allocate(
                    vertices->data(),
                    vertices->size() * sizeof(nifly::Vector3));
            }

            f->glEnableVertexAttribArray(0);
            f->glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(nifly::Vector3), nullptr);
            vertexBuffer->release();
        }
    }

    if (niShape->HasNormals()) {
        normalBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (normalBuffer->create()) {
            normalBuffer->bind();

            if (auto normals = nifFile->GetNormalsForShape(niShape)) {
                normalBuffer->allocate(
                    normals->data(),
                    normals->size() * sizeof(nifly::Vector3));
            }

            f->glEnableVertexAttribArray(1);
            f->glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(nifly::Vector3), nullptr);
            normalBuffer->release();
        }
    }

    if (niShape->HasTangents()) {
        tangentBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (tangentBuffer->create()) {
            tangentBuffer->bind();

            if (auto tangents = nifFile->GetTangentsForShape(niShape)) {
                tangentBuffer->allocate(
                    tangents->data(),
                    tangents->size() * sizeof(nifly::Vector3));
            }

            f->glEnableVertexAttribArray(2);
            f->glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(nifly::Vector3), nullptr);
            tangentBuffer->release();
        }

        bitangentBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (bitangentBuffer->create()) {
            bitangentBuffer->bind();

            if (auto bitangents = nifFile->GetBitangentsForShape(niShape)) {
                bitangentBuffer->allocate(
                    bitangents->data(),
                    bitangents->size() * sizeof(nifly::Vector3));
            }

            f->glEnableVertexAttribArray(3);
            f->glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(nifly::Vector3), nullptr);
            bitangentBuffer->release();
        }
    }

    if (niShape->HasUVs()) {
        texCoordBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (texCoordBuffer->create()) {
            texCoordBuffer->bind();

            if (auto texCoords = nifFile->GetUvsForShape(niShape)) {
                texCoordBuffer->allocate(
                    texCoords->data(),
                    texCoords->size() * sizeof(nifly::Vector2));
            }

            f->glEnableVertexAttribArray(4);
            f->glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(nifly::Vector2), nullptr);
            texCoordBuffer->release();
        }
    }

    if (!niShape->HasVertexColors()) {
        niShape->SetVertexColors(true);
    }

    colorBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
    if (colorBuffer->create()) {
        colorBuffer->bind();

        std::vector<nifly::Color4> colors;
        if (nifFile->GetColorsForShape(niShape, colors)) {
            colorBuffer->allocate(
                colors.data(),
                colors.size() * sizeof(nifly::Color4));
        }

        f->glEnableVertexAttribArray(5);
        f->glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(nifly::Color4), nullptr);
        colorBuffer->release();
    }

    indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    if (indexBuffer->create()) {
        indexBuffer->bind();

        std::vector<nifly::Triangle> tris;
        if (niShape->GetTriangles(tris)) {
            indexBuffer->allocate(tris.data(), tris.size() * sizeof(nifly::Triangle));
        }

        elements = niShape->GetNumTriangles() * 3;
        indexBuffer->release();
    }

    if (shader) {
        if (shader->HasTextureSet()) {
            auto textureSetRef = shader->TextureSetRef();
            auto textureSet = nifFile->GetHeader().GetBlock(textureSetRef);

            for (std::size_t i = 0; i < textureSet->textures.size(); i++) {
                auto texturePath = textureSet->textures[i].get();
                if (!texturePath.empty()) {
                    textures[i] = textureManager->getTexture(texturePath);
                }
                else {
                    switch (i) {
                    case TextureSlot::BaseMap:
                        textures[i] = textureManager->getBlackTexture();
                        break;
                    case TextureSlot::NormalMap:
                        textures[i] = textureManager->getFlatNormalTexture();
                        break;
                    case TextureSlot::GlowMap:
                        if (shader->HasGlowmap()) {
                            textures[i] = textureManager->getBlackTexture();
                        }
                        else {
                            textures[i] = textureManager->getWhiteTexture();
                        }
                        break;
                    default:
                        textures[i] = nullptr;
                        break;
                    }
                }
            }
        }

        specColor = convertVector3(shader->GetSpecularColor());
        specStrength = shader->GetSpecularStrength();
        specGlossiness = qBound(0.0f, shader->GetGlossiness(), 128.0f);
        hasGlowMap = shader->HasGlowmap();
        glowColor = convertColor(shader->GetEmissiveColor());
        glowMult = shader->GetEmissiveMultiple();
        alpha = shader->GetAlpha();
        tintColor = { 1.0f, 1.0f, 1.0f };
        uvScale = convertVector2(shader->GetUVScale());
        uvOffset = convertVector2(shader->GetUVOffset());
        hasEmit = shader->IsEmissive();
        hasSoftlight = shader->HasSoftlight();
        hasBacklight = shader->HasBacklight();
        hasRimlight = shader->HasRimlight();
        hasTintColor = shader->IsSkinTinted();
        lightingEffect1 = shader->GetSoftlight();
        lightingEffect2 = shader->GetRimlightPower();
        envReflection = shader->GetEnvironmentMapScale();

        if (auto alphaProperty = nifFile->GetAlphaProperty(niShape)) {
            alphaThreshold = alphaProperty->threshold / 255.0f;
        }
    }
}

void OpenGLShape::destroy()
{
    if (vertexBuffer) {
        vertexBuffer->destroy();
        delete vertexBuffer;
    }

    if (normalBuffer) {
        normalBuffer->destroy();
        delete normalBuffer;
    }

    if (tangentBuffer) {
        tangentBuffer->destroy();
        delete tangentBuffer;
    }

    if (bitangentBuffer) {
        bitangentBuffer->destroy();
        delete bitangentBuffer;
    }

    if (texCoordBuffer) {
        texCoordBuffer->destroy();
        delete texCoordBuffer;
    }

    if (colorBuffer) {
        colorBuffer->destroy();
        delete colorBuffer;
    }

    if (indexBuffer) {
        indexBuffer->destroy();
        delete indexBuffer;
    }

    if (vertexArray) {
        vertexArray->destroy();
        vertexArray->deleteLater();
    }
}

void OpenGLShape::setupShaders(QOpenGLShaderProgram* program)
{
    program->setUniformValue("BaseMap", 1);
    if (textures[0]) {
        textures[0]->bind(1);
    }

    program->setUniformValue("NormalMap", 2);
    if (textures[1]) {
        textures[1]->bind(2);
    }

    program->setUniformValue("GlowMap", 3);
    program->setUniformValue("LightMask", 3);
    program->setUniformValue("hasGlowMap", hasGlowMap && textures[2] != nullptr);
    if (textures[2]) {
        textures[2]->bind(3);
    }

    program->setUniformValue("HeightMap", 4);
    program->setUniformValue("hasHeightMap", textures[3] != nullptr);
    if (textures[3]) {
        textures[3]->bind(4);
    }

    program->setUniformValue("CubeMap", 5);
    program->setUniformValue("hasCubeMap", textures[4] != nullptr);
    if (textures[4]) {
        textures[4]->bind(5);
    }

    program->setUniformValue("EnvironmentMap", 6);
    program->setUniformValue("hasEnvMask", textures[5] != nullptr);
    if (textures[5]) {
        textures[5]->bind(6);
    }

    program->setUniformValue("BacklightMap", 8);
    if (textures[7]) {
        textures[7]->bind(8);
    }

    program->setUniformValue("ambientColor", QVector4D(0.2f, 0.2f, 0.2f, 1.0f));
    program->setUniformValue("diffuseColor", QVector4D(1.0f, 1.0f, 1.0f, 1.0f));

    program->setUniformValue("alpha", alpha);
    program->setUniformValue("tintColor", tintColor);
    program->setUniformValue("uvScale", uvScale);
    program->setUniformValue("uvOffset", uvOffset);
    program->setUniformValue("specColor", specColor);
    program->setUniformValue("specStrength", specStrength);
    program->setUniformValue("specGlossiness", specGlossiness);

    program->setUniformValue("hasEmit", hasEmit);
    program->setUniformValue("hasSoftlight", hasSoftlight);
    program->setUniformValue("hasBacklight", hasBacklight);
    program->setUniformValue("hasRimlight", hasRimlight);
    program->setUniformValue("hasTintColor", hasTintColor);

    program->setUniformValue("lightingEffect1", lightingEffect1);
    program->setUniformValue("lightingEffect2", lightingEffect2);

    program->setUniformValue("envReflection", envReflection);

    auto f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_1>();
    f->glAlphaFunc(GL_GREATER, alphaThreshold);
}

QVector2D OpenGLShape::convertVector2(nifly::Vector2 vector)
{
    return { vector.u, vector.v };
}

QVector3D OpenGLShape::convertVector3(nifly::Vector3 vector)
{
    return { vector.x, vector.y, vector.z };
}

QColor OpenGLShape::convertColor(nifly::Color4 color)
{
    return QColor::fromRgbF(color.r, color.g, color.b, color.a);
}

QMatrix4x4 OpenGLShape::convertTransform(nifly::MatTransform transform)
{
    auto mat = transform.ToMatrix();
    return QMatrix4x4{
        mat[0],  mat[1],  mat[2],  mat[3],
        mat[4],  mat[5],  mat[6],  mat[7],
        mat[8],  mat[9],  mat[10], mat[11],
        mat[12], mat[13], mat[14], mat[15],
    };
}
