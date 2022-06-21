#pragma once

#include <NifFile.hpp>
#include <cstdint>
#include <cstring>

struct SLSF1
{
    SLSF1() = delete;

    enum BSLightingShaderFlags1 : std::uint32_t
    {
        Specular                = 0x00000001,
        Skinned                 = 0x00000002,
        TempRefraction          = 0x00000004,
        VertexAlpha             = 0x00000008,
        GreyscaleToPaletteColor = 0x00000010,
        GreyscaleToPaletteAlpha = 0x00000020,
        UseFalloff              = 0x00000040,
        EnvironmentMapping      = 0x00000080,
        ReceiveShadows          = 0x00000100,
        CastShadows             = 0x00000200,
        FacegenDetailMap        = 0x00000400,
        Parallax                = 0x00000800,
        ModelSpaceNormals       = 0x00001000,
        NonProjectiveShadows    = 0x00002000,
        Landscape               = 0x00004000,
        Refraction              = 0x00008000,
        FireRefraction          = 0x00010000,
        EyeEnvironmentMapping   = 0x00020000,
        HairSoftLighting        = 0x00040000,
        ScreendoorAlphaFade     = 0x00080000,
        LocalmapHideSecret      = 0x00100000,
        FaceGenRGBTint          = 0x00200000,
        OwnEmit                 = 0x00400000,
        ProjectedUV             = 0x00800000,
        MultipleTextures        = 0x01000000,
        RemappableTextures      = 0x02000000,
        Decal                   = 0x04000000,
        DynamicDecal            = 0x08000000,
        ParallaxOcclusion       = 0x10000000,
        ExternalEmittance       = 0x20000000,
        SoftEffect              = 0x40000000,
        ZBufferTest             = 0x80000000,
    };
};

struct SLSF2
{
    SLSF2() = delete;

    enum BSLightingShaderFlags2 : std::uint32_t
    {
        ZBufferWrite                = 0x00000001,
        LODLandscape                = 0x00000002,
        LODObjects                  = 0x00000004,
        NoFade                      = 0x00000008,
        DoubleSided                 = 0x00000010,
        VertexColors                = 0x00000020,
        GlowMap                     = 0x00000040,
        AssumeShadowmask            = 0x00000080,
        PackedTangent               = 0x00000100,
        MultiIndexSnow              = 0x00000200,
        VertexLighting              = 0x00000400,
        UniformScale                = 0x00000800,
        FitSlope                    = 0x00001000,
        Billboard                   = 0x00002000,
        NoLODLandBlend              = 0x00004000,
        EnvMapLightFade             = 0x00008000,
        Wireframe                   = 0x00010000,
        WeaponBlood                 = 0x00020000,
        HideOnLocalMap              = 0x00040000,
        PremultAlpha                = 0x00080000,
        CloudLOD                    = 0x00100000,
        AnisotropicLighting         = 0x00200000,
        NoTransparencyMultisampling = 0x00400000,
        Unused01                    = 0x00800000,
        MultiLayerParallax          = 0x01000000,
        SoftLighting                = 0x02000000,
        RimLighting                 = 0x04000000,
        BackLighting                = 0x08000000,
        Unused02                    = 0x10000000,
        TreeAnim                    = 0x20000000,
        EffectLighting              = 0x40000000,
        HDLODObjects                = 0x80000000,
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
