#pragma once

#include <NifFile.hpp>
#include <Geometry.hpp>

#include <QOpenGLBuffer>
#include <QOpenGLShaderProgram>
#include <QOpenGLTexture>
#include <QOpenGLVertexArrayObject>
#include <QWidget>

#include "TextureManager.h"

enum TextureSlot : std::size_t
{
    BaseMap = 0,
    NormalMap = 1,
    GlowMap = 2,
    LightMask = 2,
    HeightMap = 3,
    EnvironmentMap = 4,
    EnvironmentMask = 5,
    BacklightMap = 7,
};

struct OpenGLShape
{
public:
    OpenGLShape(
        nifly::NifFile* nifFile,
        nifly::NiShape* niShape,
        TextureManager* textureManager);

    void destroy();
    void setupShaders(QOpenGLShaderProgram* program);

    static QVector2D convertVector2(nifly::Vector2 vector);
    static QColor convertColor(nifly::Vector3 color);
    static QColor convertColor(nifly::Color4 color);
    static QMatrix4x4 convertTransform(nifly::MatTransform transform);

    QOpenGLVertexArrayObject* vertexArray = nullptr;

    QOpenGLBuffer* vertexBuffer = nullptr;
    QOpenGLBuffer* normalBuffer = nullptr;
    QOpenGLBuffer* tangentBuffer = nullptr;
    QOpenGLBuffer* bitangentBuffer = nullptr;
    QOpenGLBuffer* texCoordBuffer = nullptr;
    QOpenGLBuffer* colorBuffer = nullptr;
    QOpenGLBuffer* boneIndexBuffer = nullptr;
    QOpenGLBuffer* boneWeightBuffer = nullptr;

    QOpenGLBuffer* indexBuffer = nullptr;
    GLsizei elements = 0;

    QOpenGLTexture* textures[13] { nullptr };

    QMatrix4x4 modelMatrix;
    QColor specColor = QColorConstants::White;
    float specStrength = 1.0f ;
    float specGlossiness = 1.0f;

    bool hasGlowMap = false;
    QColor glowColor = QColorConstants::White;
    float glowMult = 1.0f;

    float alpha = 1.0f;
    QColor tintColor;

    QVector2D uvScale{ 1.0f, 1.0f };
    QVector2D uvOffset{ 0.0f, 0.0f };

    bool hasEmit = false;
    bool hasSoftlight = false;
    bool hasBacklight = false;
    bool hasRimlight = false;
    bool hasTintColor = false;

    float lightingEffect1 = 0.3f;
    float lightingEffect2 = 2.0f;
    float envReflection = 1.0f;

    float alphaThreshold = 0.0f;
};
