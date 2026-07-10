#pragma once

#include "Renderer/Assets/AssetHandle.h"
#include "Renderer/Scene/SceneLoadDesc.h"

#include "Math/Vector.h"

#include <string>
#include <vector>

namespace ElecNeko
{
    enum class MaterialAlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    enum class MaterialMediumType
    {
        None,
        Absorb,
        Scatter
    };

    struct MaterialAssetDesc
    {
        std::string name;

        Vec3 baseColor = Vec3(1.0f, 1.0f, 1.0f);
        float opacity = 1.0f;

        Vec3 emission = Vec3(0.0f, 0.0f, 0.0f);

        float metallic = 0.0f;
        float roughness = 0.5f;

        float specularTint = 0.0f;
        float specTrans = 0.0f;
        float anisotropic = 0.0f;
        float subsurface = 0.0f;

        float sheen = 0.0f;
        float sheenTint = 0.0f;

        float clearcoat = 0.0f;
        float clearcoatGloss = 1.0f;

        float ior = 1.5f;

        MaterialMediumType mediumType = MaterialMediumType::None;
        float mediumDensity = 0.0f;
        Vec3 mediumColor = Vec3(1.0f, 1.0f, 1.0f);
        float mediumAnisotropy = 0.0f;

        MaterialAlphaMode alphaMode = MaterialAlphaMode::Opaque;
        float alphaCutoff = 0.5f;

        TextureHandle baseColorTexture;
        TextureHandle normalTexture;
        TextureHandle metalRoughTexture;
        TextureHandle emissionTexture;
    };

    struct MaterialAsset
    {
        MaterialAssetDesc desc;
    };

    struct MaterialSet
    {
        std::vector<MaterialHandle> slots;
    };

    MaterialAlphaMode ConvertSceneAlphaMode(SceneAlphaMode mode);
    MaterialMediumType ConvertSceneMediumType(SceneMediumType mode);
} // namespace ElecNeko
