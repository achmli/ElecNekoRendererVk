#pragma once

#include <cstdint>

namespace RHI
{
    enum class Backend
    {
        Vulkan,
        Metal,
        D3D12
    };

    enum class Format
    {
        Unknown,
        RGBA8_UNorm,
        RGBA8_SRGB,
        RGBA16_Float,
        RGBA32_Float,
        D32_Float
    };

    enum class BufferUsage : uint32_t
    {
        None = 0,
        Vertex = 1 << 0,
        Index = 1 << 1,
        Uniform = 1 << 2,
        Storage = 1 << 3,
        TransferSrc = 1 << 4,
        TransferDst = 1 << 5
    };

    inline BufferUsage operator|(BufferUsage a, BufferUsage b) { return static_cast<BufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }

    inline BufferUsage &operator|=(BufferUsage &a, BufferUsage b)
    {
        a = a | b;
        return a;
    }

    inline bool HasFlag(BufferUsage value, BufferUsage flag) { return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0; }

    enum class TextureUsage : uint32_t
    {
        None = 0,
        Sampled = 1 << 0,
        RenderTarget = 1 << 1,
        DepthStencil = 1 << 2,
        TransferSrc = 1 << 3,
        TransferDst = 1 << 4,
        Storage = 1 << 5
    };

    inline TextureUsage operator|(TextureUsage a, TextureUsage b) { return static_cast<TextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b)); }

    inline TextureUsage &operator|=(TextureUsage &a, TextureUsage b)
    {
        a = a | b;
        return a;
    }

    inline bool HasFlag(TextureUsage value, TextureUsage flag) { return (static_cast<uint32_t>(value) & static_cast<uint32_t>(flag)) != 0; }
} // namespace RHI
