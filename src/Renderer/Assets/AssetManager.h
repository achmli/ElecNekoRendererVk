// src/Renderer/Assets/AssetManager.h
#pragma once

#include "Renderer/Assets/AssetHandle.h"
#include "Renderer/Assets/MaterialAsset.h"
#include "Renderer/Assets/TextureAsset.h"
#include "Renderer/Mesh/StaticMesh.h"
#include "Renderer/Scene/SceneLoadDesc.h"

#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace ElecNeko
{
    class AssetManager
    {
    public:
        AssetManager() = default;
        ~AssetManager() = default;

        void Clear();

        TextureHandle GetOrCreateTexture(const TextureAssetDesc &desc);

        MaterialHandle CreateMaterial(const MaterialAssetDesc &desc);
        MaterialHandle CreateMaterialFromSceneDesc(const SceneMaterialDesc &sceneMaterial, const std::filesystem::path &baseDirectory);

        StaticMeshHandle AddStaticMeshAsset(const std::string &key, StaticMeshAsset &&asset);
        bool TryGetStaticMeshHandle(const std::string &key, StaticMeshHandle &outHandle) const;

        MaterialSetHandle CreateMaterialSet(const std::vector<MaterialHandle> &slots);

        const TextureAsset &GetTexture(TextureHandle handle) const;
        const MaterialAsset &GetMaterial(MaterialHandle handle) const;
        const StaticMeshAsset &GetStaticMesh(StaticMeshHandle handle) const;
        const MaterialSet &GetMaterialSet(MaterialSetHandle handle) const;

        size_t GetTextureCount() const { return m_textures.size(); }
        size_t GetMaterialCount() const { return m_materials.size(); }
        size_t GetStaticMeshCount() const { return m_staticMeshes.size(); }
        size_t GetMaterialSetCount() const { return m_materialSets.size(); }

    private:
        static std::string MakeTextureKey(const TextureAssetDesc &desc);
        static std::string MakePathKey(const std::filesystem::path &path);

    private:
        std::vector<TextureAsset> m_textures;
        std::vector<MaterialAsset> m_materials;
        std::vector<StaticMeshAsset> m_staticMeshes;
        std::vector<MaterialSet> m_materialSets;

        std::unordered_map<std::string, TextureHandle> m_textureCache;
        std::unordered_map<std::string, StaticMeshHandle> m_staticMeshCache;
    };
} // namespace ElecNeko
