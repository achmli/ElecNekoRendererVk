// src/Renderer/Assets/AssetManager.cpp
#include "Renderer/Assets/AssetManager.h"

#include <cassert>

namespace ElecNeko
{
    MaterialAlphaMode ConvertSceneAlphaMode(SceneAlphaMode mode)
    {
        switch (mode)
        {
            case SceneAlphaMode::Mask:
                return MaterialAlphaMode::Mask;
            case SceneAlphaMode::Blend:
                return MaterialAlphaMode::Blend;
            case SceneAlphaMode::Opaque:
            default:
                return MaterialAlphaMode::Opaque;
        }
    }

    MaterialMediumType ConvertSceneMediumType(SceneMediumType mode)
    {
        switch (mode)
        {
            case SceneMediumType::Absorb:
                return MaterialMediumType::Absorb;
            case SceneMediumType::Scatter:
                return MaterialMediumType::Scatter;
            case SceneMediumType::None:
            default:
                return MaterialMediumType::None;
        }
    }

    void AssetManager::Clear()
    {
        m_textures.clear();
        m_materials.clear();
        m_staticMeshes.clear();
        m_materialSets.clear();

        m_textureCache.clear();
        m_staticMeshCache.clear();
    }

    std::string AssetManager::MakePathKey(const std::filesystem::path &path)
    {
        std::error_code ec;
        std::filesystem::path normalized = std::filesystem::weakly_canonical(path, ec);

        if (ec)
        {
            normalized = path.lexically_normal();
        }

        return normalized.generic_string();
    }

    std::string AssetManager::MakeTextureKey(const TextureAssetDesc &desc)
    {
        switch (desc.sourceType)
        {
            case TextureSourceType::DefaultWhite:
                return "__default_white";
            case TextureSourceType::DefaultNormal:
                return "__default_normal";
            case TextureSourceType::DefaultMetalRough:
                return "__default_metal_rough";
            case TextureSourceType::DefaultBlack:
                return "__default_black";
            case TextureSourceType::Memory:
                return "__memory_" + desc.debugName;
            case TextureSourceType::File:
            default:
                return MakePathKey(desc.path) + (desc.srgb ? "|srgb" : "|linear");
        }
    }

    TextureHandle AssetManager::GetOrCreateTexture(const TextureAssetDesc &desc)
    {
        const std::string key = MakeTextureKey(desc);

        auto it = m_textureCache.find(key);

        if (it != m_textureCache.end())
        {
            return it->second;
        }

        TextureAsset asset{};
        asset.desc = desc;

        TextureHandle handle;
        handle.index = static_cast<uint32_t>(m_textures.size());

        m_textures.push_back(std::move(asset));
        m_textureCache.emplace(key, handle);

        return handle;
    }

    MaterialHandle AssetManager::CreateMaterial(const MaterialAssetDesc &desc)
    {
        MaterialAsset asset{};
        asset.desc = desc;

        MaterialHandle handle;
        handle.index = static_cast<uint32_t>(m_materials.size());

        m_materials.push_back(std::move(asset));

        return handle;
    }

    MaterialHandle AssetManager::CreateMaterialFromSceneDesc(const SceneMaterialDesc &sceneMaterial, const std::filesystem::path &baseDirectory)
    {
        MaterialAssetDesc desc{};
        desc.name = sceneMaterial.name;

        desc.baseColor = sceneMaterial.baseColor;
        desc.opacity = sceneMaterial.opacity;
        desc.emission = sceneMaterial.emission;

        desc.metallic = sceneMaterial.metallic;
        desc.roughness = sceneMaterial.roughness;

        desc.specularTint = sceneMaterial.specularTint;
        desc.specTrans = sceneMaterial.specTrans;
        desc.anisotropic = sceneMaterial.anisotropic;
        desc.subsurface = sceneMaterial.subsurface;

        desc.sheen = sceneMaterial.sheen;
        desc.sheenTint = sceneMaterial.sheenTint;

        desc.clearcoat = sceneMaterial.clearcoat;
        desc.clearcoatGloss = sceneMaterial.clearcoatGloss;

        desc.ior = sceneMaterial.ior;

        desc.mediumType = ConvertSceneMediumType(sceneMaterial.mediumType);
        desc.mediumDensity = sceneMaterial.mediumDensity;
        desc.mediumColor = sceneMaterial.mediumColor;
        desc.mediumAnisotropy = sceneMaterial.mediumAnisotropy;

        desc.alphaMode = ConvertSceneAlphaMode(sceneMaterial.alphaMode);
        desc.alphaCutoff = sceneMaterial.alphaCutoff;

        if (!sceneMaterial.baseColorTexture.empty())
        {
            TextureAssetDesc tex{};
            tex.sourceType = TextureSourceType::File;
            tex.path = baseDirectory / sceneMaterial.baseColorTexture;
            tex.debugName = sceneMaterial.baseColorTexture;
            tex.srgb = true;

            desc.baseColorTexture = GetOrCreateTexture(tex);
        }

        if (!sceneMaterial.normalTexture.empty())
        {
            TextureAssetDesc tex{};
            tex.sourceType = TextureSourceType::File;
            tex.path = baseDirectory / sceneMaterial.normalTexture;
            tex.debugName = sceneMaterial.normalTexture;
            tex.srgb = false;

            desc.normalTexture = GetOrCreateTexture(tex);
        }

        if (!sceneMaterial.metalRoughTexture.empty())
        {
            TextureAssetDesc tex{};
            tex.sourceType = TextureSourceType::File;
            tex.path = baseDirectory / sceneMaterial.metalRoughTexture;
            tex.debugName = sceneMaterial.metalRoughTexture;
            tex.srgb = false;

            desc.metalRoughTexture = GetOrCreateTexture(tex);
        }

        if (!sceneMaterial.emissionTexture.empty())
        {
            TextureAssetDesc tex{};
            tex.sourceType = TextureSourceType::File;
            tex.path = baseDirectory / sceneMaterial.emissionTexture;
            tex.debugName = sceneMaterial.emissionTexture;
            tex.srgb = true;

            desc.emissionTexture = GetOrCreateTexture(tex);
        }

        return CreateMaterial(desc);
    }

    StaticMeshHandle AssetManager::AddStaticMeshAsset(const std::string &key, StaticMeshAsset &&asset)
    {
        auto it = m_staticMeshCache.find(key);

        if (it != m_staticMeshCache.end())
        {
            return it->second;
        }

        StaticMeshHandle handle;
        handle.index = static_cast<uint32_t>(m_staticMeshes.size());

        m_staticMeshes.emplace_back(std::move(asset));
        m_staticMeshCache.emplace(key, handle);

        return handle;
    }

    bool AssetManager::TryGetStaticMeshHandle(const std::string &key, StaticMeshHandle &outHandle) const
    {
        auto it = m_staticMeshCache.find(key);

        if (it == m_staticMeshCache.end())
        {
            outHandle = StaticMeshHandle{};
            return false;
        }

        outHandle = it->second;
        return true;
    }

    MaterialSetHandle AssetManager::CreateMaterialSet(const std::vector<MaterialHandle> &slots)
    {
        MaterialSet set{};
        set.slots = slots;

        MaterialSetHandle handle;
        handle.index = static_cast<uint32_t>(m_materialSets.size());

        m_materialSets.push_back(std::move(set));

        return handle;
    }

    const TextureAsset &AssetManager::GetTexture(TextureHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_textures.size());

        return m_textures[handle.index];
    }

    const MaterialAsset &AssetManager::GetMaterial(MaterialHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_materials.size());

        return m_materials[handle.index];
    }

    const StaticMeshAsset &AssetManager::GetStaticMesh(StaticMeshHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_staticMeshes.size());

        return m_staticMeshes[handle.index];
    }

    const MaterialSet &AssetManager::GetMaterialSet(MaterialSetHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_materialSets.size());

        return m_materialSets[handle.index];
    }
} // namespace ElecNeko
