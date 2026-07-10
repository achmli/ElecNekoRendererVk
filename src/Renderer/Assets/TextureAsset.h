// src/Renderer/Assets/TextureAsset.h
#pragma once

#include "Renderer/Assets/AssetHandle.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ElecNeko
{
    enum class TextureSourceType
    {
        File,
        Memory,
        DefaultWhite,
        DefaultNormal,
        DefaultMetalRough,
        DefaultBlack
    };

    struct TextureAssetDesc
    {
        TextureSourceType sourceType = TextureSourceType::File;

        std::filesystem::path path;

        std::vector<uint8_t> memoryData;

        std::string debugName;

        bool srgb = true;
    };

    struct TextureAsset
    {
        TextureAssetDesc desc;

        int width = 0;
        int height = 0;
        int channels = 0;
    };
} // namespace ElecNeko
