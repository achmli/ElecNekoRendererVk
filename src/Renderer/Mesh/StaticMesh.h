#pragma once

#include "Renderer/Mesh/MeshVertex.h"

#include "RHI2/RHIBuffer.h"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace RHI
{
    class Device;
    class UploadBatch;
} // namespace RHI

namespace ElecNeko
{
    struct StaticMeshSection
    {
        uint32_t firstIndex = 0;
        uint32_t indexCount = 0;
        int32_t vertexOffset = 0;

        uint32_t materialIndex = 0;
    };

    class StaticMeshAsset
    {
    public:
        std::string name;

        std::vector<MeshVertex> vertices;
        std::vector<uint32_t> indices;
        std::vector<StaticMeshSection> sections;
    };

    class StaticMeshGPU
    {
    public:
        std::string name;

        std::unique_ptr<RHI::Buffer> vertexBufferRHI;
        std::unique_ptr<RHI::Buffer> indexBufferRHI;

        uint32_t vertexCount = 0;
        uint32_t indexCount = 0;

        std::vector<StaticMeshSection> sections;

        bool Upload(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch, const StaticMeshAsset &asset);

        void Cleanup();
    };
} // namespace ElecNeko
