// src/RHI/RHIHandle.h
#pragma once

#include <cstdint>
#include <limits>

namespace RHI
{
    struct BufferHandle
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();

        bool IsValid() const { return index != std::numeric_limits<uint32_t>::max(); }
    };

    struct TextureHandle
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();

        bool IsValid() const { return index != std::numeric_limits<uint32_t>::max(); }
    };

    struct PipelineHandle
    {
        uint32_t index = std::numeric_limits<uint32_t>::max();

        bool IsValid() const { return index != std::numeric_limits<uint32_t>::max(); }
    };
} // namespace RHI
