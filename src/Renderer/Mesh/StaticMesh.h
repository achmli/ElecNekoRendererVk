#pragma once

#include "Renderer/Mesh/MeshVertex.h"

#include "RHI/Buffer.h"
#include "RHI/RHIHandle.h"

#include <cstdint>
#include <string>
#include <vector>

class DeviceContext;

namespace ElecNeko
{
    // 一个 StaticMesh 可以有多个 section。
    // 每个 section 通常对应一个材质。
    struct StaticMeshSection
    {
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;

        uint32_t materialIndex = 0;
    };

    // CPU 资产数据。
    // 只存顶点、索引、section，不接触 Vulkan。
    class StaticMeshAsset
    {
    public:
        std::string name;

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<StaticMeshSection> sections;
    };

    // GPU 侧资源。
    // 现在先用 legacy Buffer 过渡，之后再换成 RHI::Device::CreateBuffer。
    class StaticMeshGPU
    {
    public:
        std::string name;

        RHI::BufferHandle vertexBuffer;
        RHI::BufferHandle indexBuffer;

        // 过渡字段：当前 Buffer 系统还没完全 RHI 化，所以先保留真实 Buffer。
        // 以后正式 RHI Device 做好后，这两个字段要删掉。
        Buffer legacyVertexBuffer;
        Buffer legacyIndexBuffer;

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        std::vector<StaticMeshSection> sections;

        bool Upload(DeviceContext *device, const StaticMeshAsset &asset);
        void Cleanup(DeviceContext *device);
    };
} // namespace ElecNeko
