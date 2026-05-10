#pragma once

#include "RHI/RHIHandle.h"

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

    // 渲染器真正消费的 draw 数据。
    // 新模型系统里，Mesh 不再自己 Draw，而是生成 MeshDrawCommand。
    struct MeshDrawCommand
    {
        RHI::BufferHandle vertexBuffer;
        RHI::BufferHandle indexBuffer;

        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;

        uint32_t materialIndex = 0;
        uint32_t instanceIndex = 0;

        MeshPassType passType = MeshPassType::Opaque;
        uint64_t sortKey = 0;
    };
} // namespace ElecNeko
