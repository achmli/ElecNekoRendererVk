#pragma once

#include "RHI2/RHIBuffer.h"

#include <cstdint>

namespace ElecNeko
{
    enum class MeshPassType : uint8_t
    {
        Opaque,
        Masked,
        Transparent,
        Shadow,
        DepthOnly
    };

    struct MeshDrawCommand
    {
        RHI::Buffer *vertexBuffer = nullptr;
        RHI::Buffer *indexBuffer = nullptr;

        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;

        uint32_t materialIndex = 0;
        uint32_t instanceIndex = 0;

        MeshPassType passType = MeshPassType::Opaque;
        uint64_t sortKey = 0;
    };
} // namespace ElecNeko
