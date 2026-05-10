#include "Renderer/Mesh/StaticMesh.h"

#include "RHI/DeviceContext.h"

#include <cassert>

namespace ElecNeko
{
    bool StaticMeshGPU::Upload(DeviceContext *device, const StaticMeshAsset &asset)
    {
        name = asset.name;
        vertexCount = static_cast<uint32_t>(asset.vertices.size());
        indexCount = static_cast<uint32_t>(asset.indices.size());
        sections = asset.sections;

        if (vertexCount == 0 || indexCount == 0)
        {
            return true;
        }

        const int vertexBufferSize = static_cast<int>(sizeof(MeshVertex) * asset.vertices.size());

        const int indexBufferSize = static_cast<int>(sizeof(uint32_t) * asset.indices.size());

        const bool vertexOk = legacyVertexBuffer.Allocate(device, asset.vertices.data(), vertexBufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);

        assert(vertexOk);

        const bool indexOk = legacyIndexBuffer.Allocate(device, asset.indices.data(), indexBufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);

        assert(indexOk);

        if (!vertexOk || !indexOk)
        {
            return false;
        }

        vertexBuffer = device->m_bufferRegistry.Register(&legacyVertexBuffer);
        indexBuffer = device->m_bufferRegistry.Register(&legacyIndexBuffer);

        return true;
    }

    void StaticMeshGPU::Cleanup(DeviceContext *device)
    {
        legacyVertexBuffer.Cleanup(device);
        legacyIndexBuffer.Cleanup(device);

        vertexBuffer = {};
        indexBuffer = {};

        vertexCount = 0;
        indexCount = 0;

        sections.clear();
    }
} // namespace ElecNeko
