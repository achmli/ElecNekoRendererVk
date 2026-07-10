#include "Renderer/Mesh/StaticMesh.h"

#include "RHI2/RHIDevice.h"

#include <cassert>

namespace ElecNeko
{
    bool StaticMeshGPU::Upload(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch, const StaticMeshAsset &asset)
    {
        assert(rhiDevice != nullptr);

        name = asset.name;
        vertexCount = static_cast<uint32_t>(asset.vertices.size());
        indexCount = static_cast<uint32_t>(asset.indices.size());
        sections = asset.sections;

        vertexBufferRHI.reset();
        indexBufferRHI.reset();

        if (vertexCount == 0 || indexCount == 0)
        {
            return true;
        }

        const uint64_t vertexBufferSize = static_cast<uint64_t>(sizeof(MeshVertex) * asset.vertices.size());

        const uint64_t indexBufferSize = static_cast<uint64_t>(sizeof(uint32_t) * asset.indices.size());

        RHI::BufferDesc vertexDesc{};
        vertexDesc.size = vertexBufferSize;
        vertexDesc.usage = RHI::BufferUsage::Vertex | RHI::BufferUsage::TransferDst;
        vertexDesc.cpuVisible = false;
        vertexDesc.debugName = "StaticMesh.VertexBuffer";

        vertexBufferRHI = rhiDevice->CreateBuffer(vertexDesc, asset.vertices.data(), uploadBatch);

        if (!vertexBufferRHI)
        {
            return false;
        }

        RHI::BufferDesc indexDesc{};
        indexDesc.size = indexBufferSize;
        indexDesc.usage = RHI::BufferUsage::Index | RHI::BufferUsage::TransferDst;
        indexDesc.cpuVisible = false;
        indexDesc.debugName = "StaticMesh.IndexBuffer";

        indexBufferRHI = rhiDevice->CreateBuffer(indexDesc, asset.indices.data(), uploadBatch);

        if (!indexBufferRHI)
        {
            vertexBufferRHI.reset();
            return false;
        }

        return true;
    }

    void StaticMeshGPU::Cleanup()
    {
        vertexBufferRHI.reset();
        indexBufferRHI.reset();

        vertexCount = 0;
        indexCount = 0;

        sections.clear();
    }
} // namespace ElecNeko
