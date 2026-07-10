#pragma once

#include <cstdint>
#include <vector>

namespace RHI
{
    class Buffer;
    class Texture;
    class Sampler;

    struct BufferBinding
    {
        uint32_t binding = 0;
        Buffer *buffer = nullptr;
        uint64_t offset = 0;
        uint64_t size = 0;
    };

    struct SampledTextureBinding
    {
        uint32_t binding = 0;
        Texture *texture = nullptr;
        Sampler *sampler = nullptr;
    };

    struct BindingSetDesc
    {
        std::vector<BufferBinding> uniformBuffers;
        std::vector<BufferBinding> storageBuffers;
        std::vector<SampledTextureBinding> sampledTextures;
    };
} // namespace RHI
