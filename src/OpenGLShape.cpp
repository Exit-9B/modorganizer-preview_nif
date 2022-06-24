#include "OpenGLShape.h"
#include "NifExtensions.h"

#include <QOpenGLContext>
#include <QOpenGLFunctions_2_1>

template <typename T>
inline static QOpenGLBuffer* makeVertexBuffer(const std::vector<T>* data, GLuint attrib)
{
    QOpenGLBuffer* buffer = nullptr;

    if (data) {
        buffer = new QOpenGLBuffer(QOpenGLBuffer::VertexBuffer);
        if (buffer->create() && buffer->bind()) {
            buffer->allocate(data->data(), data->size() * sizeof(T));

            auto f = QOpenGLContext::currentContext()
                ->versionFunctions<QOpenGLFunctions_2_1>();

            f->glEnableVertexAttribArray(attrib);

            f->glVertexAttribPointer(
                attrib,
                sizeof(T) / sizeof(float),
                GL_FLOAT,
                GL_FALSE,
                sizeof(T),
                nullptr);

            buffer->release();
        }
    }

    return buffer;
}

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

    auto xform = GetShapeTransformToGlobal(nifFile, niShape);
    modelMatrix = convertTransform(xform);

    f->glVertexAttrib3f(AttribNormal, 0.5f, 0.5f, 1.0f);
    f->glVertexAttrib3f(AttribTangent, 1.0f, 0.5, 0.5f);
    f->glVertexAttrib3f(AttribBitangent, 0.5f, 1.0f, 0.5f);
    f->glVertexAttrib4f(AttribColor, 1.0f, 1.0f, 1.0f, 1.0f);

    if (auto verts = nifFile->GetVertsForShape(niShape)) {
        vertexBuffers[AttribPosition] = makeVertexBuffer(verts, AttribPosition);
    }

    if (auto normals = nifFile->GetNormalsForShape(niShape)) {
        vertexBuffers[AttribNormal] = makeVertexBuffer(normals, AttribNormal);
    }

    if (auto tangents = nifFile->GetTangentsForShape(niShape)) {
        vertexBuffers[AttribTangent] = makeVertexBuffer(tangents, AttribTangent);
    }

    if (auto bitangents = nifFile->GetBitangentsForShape(niShape)) {
        vertexBuffers[AttribBitangent] = makeVertexBuffer(bitangents, AttribBitangent);
    }

    if (auto uvs = nifFile->GetUvsForShape(niShape)) {
        vertexBuffers[AttribTexCoord] = makeVertexBuffer(uvs, AttribTexCoord);
    }

    if (std::vector<nifly::Color4> colors; nifFile->GetColorsForShape(niShape, colors)) {
        vertexBuffers[AttribColor] = makeVertexBuffer(&colors, AttribColor);
    }

    indexBuffer = new QOpenGLBuffer(QOpenGLBuffer::IndexBuffer);
    if (indexBuffer->create() && indexBuffer->bind()) {

        if (std::vector<nifly::Triangle> tris; niShape->GetTriangles(tris)) {
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

            NiAlphaPropertyFlags flags = alphaProperty->flags;

            alphaBlendEnable = flags.isAlphaBlendEnabled();
            srcBlendMode = flags.sourceBlendingFactor();
            dstBlendMode = flags.destinationBlendingFactor();
            alphaTestEnable = flags.isAlphaTestEnabled();
            alphaTestMode = flags.alphaTestMode();

            alphaThreshold = alphaProperty->threshold / 255.0f;
        }

        if (auto bslsp = dynamic_cast<nifly::BSLightingShaderProperty*>(shader)) {
            zBufferTest = bslsp->shaderFlags1 & SLSF1::ZBufferTest;
            zBufferWrite = bslsp->shaderFlags2 & SLSF2::ZBufferWrite;

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
            hasWeaponBlood = effectShader->shaderFlags2 & SLSF2::WeaponBlood;
        }
    }
    else {
        textures[BaseMap] = textureManager->getWhiteTexture();
        textures[NormalMap] = textureManager->getFlatNormalTexture();
    }
}

void OpenGLShape::destroy()
{
    for (std::size_t i = 0; i < ATTRIB_COUNT; i++) {
        if (vertexBuffers[i]) {
            vertexBuffers[i]->destroy();
            delete vertexBuffers[i];
            vertexBuffers[i] = nullptr;
        }
    }

    if (indexBuffer) {
        indexBuffer->destroy();
        delete indexBuffer;
        indexBuffer = nullptr;
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

    for (std::size_t i = 0; i < ATTRIB_COUNT; i++) {
        if (vertexBuffers[i]) {
            f->glEnableVertexAttribArray(i);
        }
        else {
            f->glDisableVertexAttribArray(i);
        }
    }

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
