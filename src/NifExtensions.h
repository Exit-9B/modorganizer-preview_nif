#pragma once

#include <QOpenGLFunctions>
#include <NifFile.hpp>
#include <cstdint>
#include <cstring>

struct SLSF1
{
    SLSF1() = delete;

    enum BSLightingShaderFlags1 : std::uint32_t
    {
        Specular                = 1U << 0,
        Skinned                 = 1U << 1,
        TempRefraction          = 1U << 2,
        VertexAlpha             = 1U << 3,
        GreyscaleToPaletteColor = 1U << 4,
        GreyscaleToPaletteAlpha = 1U << 5,
        UseFalloff              = 1U << 6,
        EnvironmentMapping      = 1U << 7,
        ReceiveShadows          = 1U << 8,
        CastShadows             = 1U << 9,
        FacegenDetailMap        = 1U << 10,
        Parallax                = 1U << 11,
        ModelSpaceNormals       = 1U << 12,
        NonProjectiveShadows    = 1U << 13,
        Landscape               = 1U << 14,
        Refraction              = 1U << 15,
        FireRefraction          = 1U << 16,
        EyeEnvironmentMapping   = 1U << 17,
        HairSoftLighting        = 1U << 18,
        ScreendoorAlphaFade     = 1U << 19,
        LocalmapHideSecret      = 1U << 20,
        FaceGenRGBTint          = 1U << 21,
        OwnEmit                 = 1U << 22,
        ProjectedUV             = 1U << 23,
        MultipleTextures        = 1U << 24,
        RemappableTextures      = 1U << 25,
        Decal                   = 1U << 26,
        DynamicDecal            = 1U << 27,
        ParallaxOcclusion       = 1U << 28,
        ExternalEmittance       = 1U << 29,
        SoftEffect              = 1U << 30,
        ZBufferTest             = 1U << 31,
    };
};

struct SLSF2
{
    SLSF2() = delete;

    enum BSLightingShaderFlags2 : std::uint32_t
    {
        ZBufferWrite                = 1U << 0,
        LODLandscape                = 1U << 1,
        LODObjects                  = 1U << 2,
        NoFade                      = 1U << 3,
        DoubleSided                 = 1U << 4,
        VertexColors                = 1U << 5,
        GlowMap                     = 1U << 6,
        AssumeShadowmask            = 1U << 7,
        PackedTangent               = 1U << 8,
        MultiIndexSnow              = 1U << 9,
        VertexLighting              = 1U << 10,
        UniformScale                = 1U << 11,
        FitSlope                    = 1U << 12,
        Billboard                   = 1U << 13,
        NoLODLandBlend              = 1U << 14,
        EnvMapLightFade             = 1U << 15,
        Wireframe                   = 1U << 16,
        WeaponBlood                 = 1U << 17,
        HideOnLocalMap              = 1U << 18,
        PremultAlpha                = 1U << 19,
        CloudLOD                    = 1U << 20,
        AnisotropicLighting         = 1U << 21,
        NoTransparencyMultisampling = 1U << 22,
        Unused01                    = 1U << 23,
        MultiLayerParallax          = 1U << 24,
        SoftLighting                = 1U << 25,
        RimLighting                 = 1U << 26,
        BackLighting                = 1U << 27,
        Unused02                    = 1U << 28,
        TreeAnim                    = 1U << 29,
        EffectLighting              = 1U << 30,
        HDLODObjects                = 1U << 31,
    };
};

struct TriShape
{
    TriShape() = delete;

    enum NiAVObjectFlags : std::uint32_t
    {
        Hidden                            = 1U << 0,
        SelectiveUpdate                   = 1U << 1,
        SelectiveUpdateTransforms         = 1U << 2,
        SelectiveUpdateController         = 1U << 3,
        SelectiveUpdateRigid              = 1U << 4,
        DisplayUIObject                   = 1U << 5,
        DisableSorting                    = 1U << 6,
        SelectiveUpdateTransformsOverride = 1U << 7,
        SaveExternalGeomData              = 1U << 9,
        NoDecals                          = 1U << 10,
        AlwaysDraw                        = 1U << 11,
        MeshLODFO4                        = 1U << 12,
        FixedBound                        = 1U << 13,
        TopFadeNode                       = 1U << 14,
        IgnoreFade                        = 1U << 15,
        NoAnimSyncX                       = 1U << 16,
        NoAnimSyncY                       = 1U << 17,
        NoAnimSyncZ                       = 1U << 18,
        NoAnimSyncS                       = 1U << 19,
        NoDismember                       = 1U << 20,
        NoDismemberValidity               = 1U << 21,
        RenderUse                         = 1U << 22,
        MaterialsApplied                  = 1U << 23,
        HighDetail                        = 1U << 24,
        ForceUpdate                       = 1U << 25,
        PreProcessedNode                  = 1U << 26,
        MeshLODSkyrim                     = 1U << 27,
    };
};

struct NiAlphaPropertyFlags
{
public:
    NiAlphaPropertyFlags(std::uint16_t flags = 0)
    {
        std::memcpy(this, &flags, sizeof(NiAlphaPropertyFlags));
    }

    bool isAlphaBlendEnabled()
    {
        return m_AlphaBlendEnable;
    }

    GLenum sourceBlendingFactor()
    {
        return getBlendMode(m_SrcBlendMode);
    }

    GLenum destinationBlendingFactor()
    {
        return getBlendMode(m_DstBlendMode);
    }

    bool isAlphaTestEnabled()
    {
        return m_AlphaTestEnable;
    }

    GLenum alphaTestMode()
    {
        return getTestMode(m_AlphaTestMode);
    }

    bool isTriangleSortDisabled()
    {
        return m_NoSort;
    }

private:
    static std::uint32_t getBlendMode(std::uint16_t flags)
    {
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
        };
    }

    static std::uint32_t getTestMode(std::uint16_t flags)
    {
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

    std::uint16_t m_AlphaBlendEnable : 1;
    std::uint16_t m_SrcBlendMode : 4;
    std::uint16_t m_DstBlendMode : 4;
    std::uint16_t m_AlphaTestEnable : 1;
    std::uint16_t m_AlphaTestMode : 3;
    std::uint16_t m_NoSort : 1;
};
static_assert(sizeof(NiAlphaPropertyFlags) == 2);

inline nifly::MatTransform GetShapeTransformToGlobal(
    nifly::NifFile* nifFile,
    nifly::NiShape* niShape)
{
    nifly::MatTransform xform = niShape->GetTransformToParent();
    nifly::NiNode* parent = nifFile->GetParentNode(niShape);
    while (parent) {
        xform = parent->GetTransformToParent().ComposeTransforms(xform);
        parent = nifFile->GetParentNode(parent);
    }

    return xform;
}

inline nifly::BoundingSphere GetBoundingSphere(
    nifly::NifFile* nifFile,
    nifly::NiShape* niShape)
{
    if (auto vertices = nifFile->GetVertsForShape(niShape)) {
        auto bounds = nifly::BoundingSphere(*vertices);

        auto xform = GetShapeTransformToGlobal(nifFile, niShape);

        bounds.center = xform.ApplyTransform(bounds.center);
        bounds.radius = xform.ApplyTransformToDist(bounds.radius);
        return bounds;
    }
    else {
        return nifly::BoundingSphere();
    }
}
