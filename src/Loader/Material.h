#pragma once

#include "Math/Vector.h"

#include <string>

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

    struct Material_t
    {
        float baseColor[3];
        float anisotropic;

        float emission[3];
        float padding0;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;

        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatGloss;

        float specTrans;
        float ior;
        uint32_t mediumType;
        float mediumDensity;

        float mediumColor[3];
        float mediumAnisotropy;

        int32_t baseColorTexId;
        int32_t metallicRoughtnessTexId;
        int32_t normalMapTexId;
        int32_t emissionmapTexId;

        float opacity;
        uint32_t alphaMode;
        float alphaCutoff;
        float padding1;
    };

    class Material
    {
    public:
        Material()
        {
            name = "";

            baseColor = Vec3(1.f, 1.f, 1.f);
            anisotropic = 0.f;

            emission = Vec3(0.f, 0.f, 0.f);
            padding0 = 0.f;

            metallic = 0.f;
            roughness = 0.5f;
            subsurface = 0.f;
            specularTint = 0.f;

            sheen = 0.f;
            sheenTint = 0.0f;
            clearcoat = 0.f;
            clearcoatGloss = 0.f;

            specTrans = 0.f;
            ior = 1.5f;
            mediumType = MediumType::None;
            mediumDensity = 0.f;

            mediumColor = Vec3(1.f, 1.f, 1.f);
            mediumAnisotropy = 0.f;

            baseColorTexId = -1;
            metallicRoughtnessTexId = -1;
            normalMapTexId = -1;
            emissionmapTexId = -1;

            opacity = 1.f;
            alphaMode = AlphaMode::Opaque;
            alphaCutoff = 0.f;
            padding1 = 0.f;
        }

        Material_t MakeStrcut();
        bool MakeBuffer(DeviceContext *device);

    public:
        std::string name;

        Vec3 baseColor;
        float anisotropic;

        Vec3 emission;
        float padding0;

        float metallic;
        float roughness;
        float subsurface;
        float specularTint;

        float sheen;
        float sheenTint;
        float clearcoat;
        float clearcoatGloss;

        float specTrans;
        float ior;
        int mediumType;
        float mediumDensity;

        Vec3 mediumColor;
        float mediumAnisotropy;

        int baseColorTexId;
        int metallicRoughtnessTexId;
        int normalMapTexId;
        int emissionmapTexId;

        float opacity;
        int alphaMode;
        float alphaCutoff;
        float padding1;

        Buffer matBuffer;
    };

    inline Material_t Material::MakeStrcut()
    {
        Material_t mat;
        mat.baseColor[0] = baseColor.x;
        mat.baseColor[1] = baseColor.y;
        mat.baseColor[2] = baseColor.z;

        mat.anisotropic = anisotropic;

        mat.emission[0] = emission.x;
        mat.emission[1] = emission.y;
        mat.emission[2] = emission.z;

        mat.padding0 = padding0;

        mat.metallic = metallic;
        mat.roughness = roughness;
        mat.subsurface = subsurface;
        mat.specularTint = specularTint;

        mat.sheen = sheen;
        mat.sheenTint = sheenTint;
        mat.clearcoat = clearcoat;
        mat.clearcoatGloss = clearcoatGloss;

        mat.specTrans = specTrans;
        mat.ior = ior;
        mat.mediumType = mediumType;
        mat.mediumDensity;

        mat.mediumColor[0] = mediumColor.x;
        mat.mediumColor[1] = mediumColor.y;
        mat.mediumColor[2] = mediumColor.z;

        mat.mediumAnisotropy = mediumAnisotropy;

        mat.baseColorTexId = baseColorTexId;
        mat.metallicRoughtnessTexId = metallicRoughtnessTexId;
        mat.normalMapTexId = normalMapTexId;
        mat.emissionmapTexId = emissionmapTexId;

        mat.opacity = opacity;
        mat.alphaMode = alphaMode;
        mat.alphaCutoff = alphaCutoff;
        mat.padding1 = padding1;

        return mat;
    }

    inline bool Material::MakeBuffer(DeviceContext *device)
    {
        Material_t mate = MakeStrcut();

        if (!matBuffer.Allocate(device, &mate, sizeof(Material_t), VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        {
            std::cerr << "Failed to Allocate Material Uniform Buffer:" << name << "\n";
            return false;
        }

        return true;
    }
} // namespace ElecNeko
