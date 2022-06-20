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
        if (shader && shader->HasType<nifly::BSEffectShaderProperty>()) {
            shaderType = ShaderManager::FO4EffectShader;
        }
        else {
            shaderType = ShaderManager::FO4Default;
        }
    }
    else {
        if (shader && shader->HasType<nifly::BSEffectShaderProperty>()) {
            shaderType = ShaderManager::SKEffectShader;
        }
        else {
            if (shader && shader->IsModelSpace()) {
                shaderType = ShaderManager::SKMSN;
            }
            else if (shader && shader->GetShaderType() == nifly::BSLSP_MULTILAYERPARALLAX) {
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

    nifly::MatTransform xform = niShape->GetTransformToParent();
    nifly::NiNode* parent = nifFile->GetParentNode(niShape);
    while (parent) {
        xform = parent->GetTransformToParent().ComposeTransforms(xform);
        parent = nifFile->GetParentNode(parent);
    }
    modelMatrix = convertTransform(xform);

    f->glVertexAttrib3f(AttribNormal, 0.5f, 0.5f, 1.0f);
    f->glVertexAttrib3f(AttribTangent, 1.0f, 0.5, 0.5f);
    f->glVertexAttrib3f(AttribBitangent, 0.5f, 1.0f, 0.5f);
    f->glVertexAttrib4f(AttribColor, 1.0f, 1.0f, 1.0f, 1.0f);

    if (auto vertices = nifFile->GetVertsForShape(niShape)) {
        vertexBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (vertexBuffer->create()) {
            vertexBuffer->bind();

            vertexBuffer->allocate(
                vertices->data(),
                vertices->size() * sizeof(nifly::Vector3));

            f->glEnableVertexAttribArray(AttribPosition);
            f->glVertexAttribPointer(
                AttribPosition,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Vector3),
                nullptr);
            }

            vertexBuffer->release();
    }

    if (auto normals = nifFile->GetNormalsForShape(niShape)) {
        normalBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (normalBuffer->create()) {
            normalBuffer->bind();

            normalBuffer->allocate(
                normals->data(),
                normals->size() * sizeof(nifly::Vector3));

            f->glEnableVertexAttribArray(AttribNormal);
            f->glVertexAttribPointer(
                AttribNormal,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Vector3),
                nullptr);

            normalBuffer->release();
        }
    }

    if (auto tangents = nifFile->GetTangentsForShape(niShape)) {
        tangentBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (tangentBuffer->create()) {
            tangentBuffer->bind();

            tangentBuffer->allocate(
                tangents->data(),
                tangents->size() * sizeof(nifly::Vector3));

            f->glEnableVertexAttribArray(AttribTangent);
            f->glVertexAttribPointer(
                AttribTangent,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Vector3),
                nullptr);

            tangentBuffer->release();
        }
    }

    if (auto bitangents = nifFile->GetBitangentsForShape(niShape)) {
        bitangentBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (bitangentBuffer->create()) {
            bitangentBuffer->bind();

            bitangentBuffer->allocate(
                bitangents->data(),
                bitangents->size() * sizeof(nifly::Vector3));

            f->glEnableVertexAttribArray(AttribBitangent);
            f->glVertexAttribPointer(
                AttribBitangent,
                3,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Vector3),
                nullptr);

            bitangentBuffer->release();
        }
    }

    if (auto texCoords = nifFile->GetUvsForShape(niShape)) {
        texCoordBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (texCoordBuffer->create()) {
            texCoordBuffer->bind();

            texCoordBuffer->allocate(
                texCoords->data(),
                texCoords->size() * sizeof(nifly::Vector2));

            f->glEnableVertexAttribArray(AttribTexCoord);
            f->glVertexAttribPointer(
                AttribTexCoord,
                2,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Vector2),
                nullptr);

            texCoordBuffer->release();
        }
    }

    std::vector<nifly::Color4> colors;
    if (nifFile->GetColorsForShape(niShape, colors)) {
        colorBuffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (colorBuffer->create()) {
            colorBuffer->bind();

            colorBuffer->allocate(
                colors.data(),
                colors.size() * sizeof(nifly::Color4));

            f->glEnableVertexAttribArray(AttribColor);
            f->glVertexAttribPointer(
                AttribColor,
                4,
                GL_FLOAT,
                GL_FALSE,
                sizeof(nifly::Color4),
                nullptr);

            colorBuffer->release();
        }
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

                if (textures[i] == nullptr) {
                    switch (i) {
                    case TextureSlot::BaseMap:
                        textures[i] = textureManager->getErrorTexture();
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
        fresnelPower = shader->GetFresnelPower();
        paletteScale = shader->GetGrayscaleToPaletteScale();

        hasGlowMap = shader->HasGlowmap();
        glowColor = convertColor(shader->GetEmissiveColor());
        glowMult = shader->GetEmissiveMultiple();

        alpha = shader->GetAlpha();
        uvScale = convertVector2(shader->GetUVScale());
        uvOffset = convertVector2(shader->GetUVOffset());

        hasEmit = shader->IsEmissive();
        hasSoftlight = shader->HasSoftlight();
        hasBacklight = shader->HasBacklight();
        hasRimlight = shader->HasRimlight();

        softlight = shader->GetSoftlight();
        backlightPower = shader->GetBacklightPower();
        rimPower = shader->GetRimlightPower();
        doubleSided = shader->IsDoubleSided();
        envReflection = shader->GetEnvironmentMapScale();

        if (auto alphaProperty = nifFile->GetAlphaProperty(niShape)) {

            static auto getBlendMode =
                [](int flags) {
                    switch (flags) {
                    case 0: return GL_ONE;
                    case 1: return GL_ZERO;
                    case 2: return GL_SRC_COLOR;
                    case 3: return GL_ONE_MINUS_SRC_COLOR;
                    case 4: return GL_DST_COLOR;
                    case 5: return GL_ONE_MINUS_DST_COLOR;
                    case 6: return GL_SRC_ALPHA;
                    case 7: return GL_ONE_MINUS_SRC_ALPHA;
                    case 8: return GL_DST_ALPHA;
                    case 9: return GL_ONE_MINUS_DST_ALPHA;
                    default: return GL_ONE;
                    }
                };

            static auto getTestMode =
                [](int flags) {
                    switch (flags) {
                    case 0: return GL_ALWAYS;
                    case 1: return GL_LESS;
                    case 2: return GL_EQUAL;
                    case 3: return GL_LEQUAL;
                    case 4: return GL_GREATER;
                    case 5: return GL_NOTEQUAL;
                    case 6: return GL_GEQUAL;
                    case 7: return GL_NEVER;
                    default: return GL_ALWAYS;
                    }
                };

            auto& flags = alphaProperty->flags;
            alphaBlendEnable = bool((flags >> 0) & 0x1);
            srcBlendMode = getBlendMode((flags >> 1) & 0xF);
            dstBlendMode = getBlendMode((flags >> 5) & 0xF);
            alphaTestEnable = bool((flags >> 9) & 0x1);
            alphaTestMode = getTestMode((flags >> 10) & 0x7);

            alphaThreshold = alphaProperty->threshold / 255.0f;
        }

        if (auto bslsp = dynamic_cast<nifly::BSLightingShaderProperty*>(shader)) {
            zBufferWrite = bool(bslsp->shaderFlags2 & 0x00000001);
            zBufferTest = bool(bslsp->shaderFlags1 & 0x80000000);

            auto bslspType = bslsp->GetShaderType();
            if (bslspType == nifly::BSLSP_SKINTINT || bslspType == nifly::BSLSP_FACE) {
                tintColor = convertVector3(bslsp->skinTintColor);
                hasTintColor = true;
            }
            else if (bslspType == nifly::BSLSP_HAIRTINT) {
                tintColor = convertVector3(bslsp->hairTintColor);
                hasTintColor = true;
            }

            if (bslspType == nifly::BSLSP_MULTILAYERPARALLAX) {
                innerScale = convertVector2(bslsp->parallaxInnerLayerTextureScale);
                innerThickness = bslsp->parallaxInnerLayerThickness;
                outerRefraction = bslsp->parallaxRefractionScale;
                outerReflection = bslsp->parallaxEnvmapStrength;
            }
        }

        if (auto effectShader = dynamic_cast<nifly::BSEffectShaderProperty*>(shader)) {
            hasWeaponBlood = effectShader->shaderFlags2 & 0x00020000;
        }
    }
    else {
        textures[BaseMap] = textureManager->getWhiteTexture();
        textures[NormalMap] = textureManager->getFlatNormalTexture();
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
    program->setUniformValue("BaseMap", BaseMap + 1);
    program->setUniformValue("NormalMap", NormalMap + 1);
    program->setUniformValue("GlowMap", GlowMap + 1);
    program->setUniformValue("LightMask", LightMask + 1);
    program->setUniformValue("hasGlowMap", hasGlowMap && textures[GlowMap] != nullptr);
    program->setUniformValue("HeightMap", HeightMap + 1);
    program->setUniformValue("hasHeightMap", textures[HeightMap] != nullptr);
    program->setUniformValue("DetailMask", DetailMask + 1);
    program->setUniformValue("hasDetailMask", textures[DetailMask] != nullptr);
    program->setUniformValue("CubeMap", EnvironmentMap + 1);
    program->setUniformValue("hasCubeMap", textures[EnvironmentMap] != nullptr);
    program->setUniformValue("EnvironmentMap", EnvironmentMask + 1);
    program->setUniformValue("hasEnvMask", textures[EnvironmentMask] != nullptr);
    program->setUniformValue("TintMask", TintMask + 1);
    program->setUniformValue("hasTintMask", textures[TintMask] != nullptr);
    program->setUniformValue("InnerMap", InnerMap + 1);
    program->setUniformValue("BacklightMap", BacklightMap + 1);
    program->setUniformValue("SpecularMap", SpecularMap + 1);
    program->setUniformValue("hasSpecularMap", textures[SpecularMap] != nullptr);

    for (int i = 0; i < textures.size(); i++) {
        if (textures[i]) {
            textures[i]->bind(i + 1);
        }
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
    program->setUniformValue("fresnelPower", fresnelPower);

    program->setUniformValue("paletteScale", paletteScale);

    program->setUniformValue("hasEmit", hasEmit);
    program->setUniformValue("hasSoftlight", hasSoftlight);
    program->setUniformValue("hasBacklight", hasBacklight);
    program->setUniformValue("hasRimlight", hasRimlight);
    program->setUniformValue("hasTintColor", hasTintColor);
    program->setUniformValue("hasWeaponBlood", hasWeaponBlood);

    program->setUniformValue("softlight", softlight);
    program->setUniformValue("backlightPower", backlightPower);
    program->setUniformValue("rimPower", rimPower);
    program->setUniformValue("subsurfaceRolloff", subsurfaceRolloff);
    program->setUniformValue("doubleSided", doubleSided);

    program->setUniformValue("envReflection", envReflection);

    if (shaderType == ShaderManager::SKMultilayer) {
        program->setUniformValue("innerScale", innerScale);
        program->setUniformValue("innerThickness", innerThickness);
        program->setUniformValue("outerRefraction", outerRefraction);
        program->setUniformValue("outerReflection", outerReflection);
    }

    auto f = QOpenGLContext::currentContext()->versionFunctions<QOpenGLFunctions_2_1>();

    f->glDepthMask(zBufferWrite ? GL_TRUE : GL_FALSE);

    if (zBufferTest) {
        f->glEnable(GL_DEPTH_TEST);
    }
    else {
        f->glDisable(GL_DEPTH_TEST);
    }

    if (doubleSided) {
        f->glDisable(GL_CULL_FACE);
    }
    else {
        f->glEnable(GL_CULL_FACE);
        f->glCullFace(GL_BACK);
    }

    if (alphaBlendEnable) {
        f->glEnable(GL_BLEND);
        f->glBlendFunc(srcBlendMode, dstBlendMode);
    }
    else {
        f->glDisable(GL_BLEND);
    }

    if (alphaTestEnable) {
        f->glEnable(GL_ALPHA_TEST);
        f->glAlphaFunc(alphaTestMode, alphaThreshold);
    }
    else {
        f->glDisable(GL_ALPHA_TEST);
    }
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
