#pragma once

#include "Math/Vector.h"

namespace ElecNeko
{
    enum AlphaMode
    {
        Opaque,
        Blend,
        Mask
    };

    enum MediumType
    {
        None,
        Absorb,
        Scatter,
        Emissive
    };

    class Material
    {
    public:
        Material();

    public:
        Vec3 baseColor;
        float anisotropic;

        Vec3 emission;
        float padding1;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;

        float sheen;
        float sheenTint;
        float clearCoat;
        float clearCoatGloss;

        float specTrans;
        float ior;
        float mediumType;
        float mediumDensity;

        Vec3 mediumColor;
        float mediumAnisotropy;

        float baseColorTexId;
        float metallicRoughnessTexId;
        float normalMapTexId;
        float emissionMapTexId;

        float opacity;
        float alphaMode;
        float alphaCutoff;
        float padding2;
    };

    inline Material::Material()
    {
        baseColor = Vec3(1.f, 1.f, 1.f);
        anisotropic = 0.f;

        emission = Vec3(0.f, 0.f, 0.f);

        padding1 = 0;

        metallic = 0.f;
        roughness = .5f;
        subsurface = 0.f;
        specularTint = 0.f;

        sheen = 0.f;
        sheenTint = 0.f;
        clearCoat = 0.f;
        clearCoatGloss = 0.f;

        specTrans = 0.f;
        ior = 1.5f;
        mediumType = 0.f;
        mediumDensity = 0.f;

        mediumColor = Vec3(1.f, 1.f, 1.f);
        mediumAnisotropy = 0.f;

        baseColorTexId = -1.f;
        metallicRoughnessTexId = -1.f;
        normalMapTexId = -1.f;
        emissionMapTexId = -1.f;

        opacity = 1.f;
        alphaMode = 0.f;
        alphaCutoff = 0.f;

        padding2 = 0;
    }

} // namespace ElecNeko
