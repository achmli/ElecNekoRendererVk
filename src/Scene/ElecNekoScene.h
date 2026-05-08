#pragma once

#include "Math/Matrix.h"
#include "RHI/Buffer.h"

#include <unordered_map>

namespace ElecNeko
{
    //=======================================================
    // already defined by Vulkan SDK
    //=======================================================
    // typedef struct VkDrawIndexedIndirectCommand {
    //     uint32_t    indexCount;
    //     uint32_t    instanceCount;
    //     uint32_t    firstIndex;
    //     int32_t     vertexOffset;
    //     uint32_t    firstInstance;
    // } VkDrawIndexedIndirectCommand;

    struct NeoVertex
    {
        float position[3];
        float normal[3];
        float uv[2];

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bd{};
            bd.binding = 0;
            bd.stride = sizeof(NeoVertex);
            bd.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
            return bd;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> a;
            a[0] = {0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NeoVertex, position)};
            a[1] = {1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(NeoVertex, normal)};
            a[2] = {2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(NeoVertex, uv)};
            return a;
        }
    };

    struct InstanceGPU
    {
        alignas(16) Mat4 modelMatrix;
        uint32_t materialId;
        uint32_t meshId;
        uint32_t flags;
        uint32_t padding;
    };

    struct SubmeshRange
    {
        uint32_t indexOffset;
        uint32_t indexCount;
        int32_t vertexOffset;
        int materialId;
    };

    class ElecNekoModel
    {
    public:
        std::string name;
        std::vector<NeoVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<SubmeshRange> m_submeshes;

        Buffer vertexBuffer;
        Buffer indexBuffer;

        void LoadMesh(DeviceContext *device, const std::string &filename);

        bool MakeVBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);

        void DrawSubmeshInstanced(VkCommandBuffer cmd, uint32_t submeshIndex, uint32_t instanceCount, uint32_t firstInstance);
    };

    struct DrawBatch
    {
        int meshId;
        int submeshIndex;
        int materialId;
        uint32_t firstInstance;
        uint32_t instanceCount;

        VkDrawIndexedIndirectCommand indirectCommand;
    };

    class ElecNekoScene
    {
    public:
        bool Initialize(DeviceContext *device);
        void Cleanup(DeviceContext *device);

        void BuildBatches(DeviceContext *device, std::vector<InstanceGPU> &instances);

        void UploadInstanceBuffer(DeviceContext *device);

        void BuildIndirectBuffer(DeviceContext *device);

        void RecordDrawCommands(VkCommandBuffer cmd, bool useIndirect);

    public:
        std::vector<std::unique_ptr<ElecNekoModel>> m_meshes;
        std::vector<InstanceGPU> m_packedInstances; // CPU-side packing, upload to GPU
        std::vector<DrawBatch> m_batches;

        Buffer instanceBufferGPU; // VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | STORAGE
        Buffer indirectBufferGPU; // VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT | STORAGE
        Buffer indirectBufferStaging; // host-visible staging buffer for indirect CPU patch
    };
} // namespace ElecNeko
