// src/Renderer/Assets/AssetHandle.h
#pragma once

#include <cstdint>
#include <limits>

namespace ElecNeko
{
    template<typename Tag>
    struct AssetHandle
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();

        bool IsValid() const { return index != std::numeric_limits<uint32_t>::max(); }

        bool operator==(const AssetHandle &rhs) const { return index == rhs.index; }

        bool operator!=(const AssetHandle &rhs) const { return index != rhs.index; }
    };

    struct TextureAssetTag;
    struct MaterialAssetTag;
    struct StaticMeshAssetTag;
    struct MaterialSetTag;

    using TextureHandle = AssetHandle<TextureAssetTag>;
    using MaterialHandle = AssetHandle<MaterialAssetTag>;
    using StaticMeshHandle = AssetHandle<StaticMeshAssetTag>;
    using MaterialSetHandle = AssetHandle<MaterialSetTag>;
} // namespace ElecNeko
