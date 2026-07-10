#pragma once

#include "RHI2/RHICommon.h"

#include <cstdint>

namespace RHI
{
    struct TextureDesc
    {
        uint32_t width = 1;
        uint32_t height = 1;
        uint32_t depth = 1;
        uint32_t layers = 1;
        uint32_t mipLevels = 1;

        Format format = Format::Unknown;
        TextureUsage usage = TextureUsage::None;

        bool force2DArrayView = false;

        const char *debugName = nullptr;
    };

    class Texture
    {
    public:
        virtual ~Texture() = default;

        virtual const TextureDesc &GetDesc() const = 0;
    };
} // namespace RHI
