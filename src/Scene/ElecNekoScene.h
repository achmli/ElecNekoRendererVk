#pragma once

#include "Light.h"
#include "Loader/Mesh.h"

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

    struct Submesh
    {
        uint32_t indexCount;
        uint32_t firstIndex;
        int32_t vertexOffset;
        uint32_t materialID;
    };

    struct InstanceData
    {
        Mat4 modelMatrix;
        Mat4 normalMatrix;
        uint32_t materialInstance;
        uint32_t padding[3];
    };

    class ElecNekoModel
    {
    public:
        ElecNekoModel() = default;
        ~ElecNekoModel() = default;

        bool MakeVBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);

    public:
        std::vector<VVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<Submesh> m_submeshes;

        Buffer m_vertexBuffer;
        Buffer m_indexBuffer;
    };

    struct ElecNekoModelInstance
    {
        ElecNekoModel *model = nullptr;
        InstanceData data;
        bool visible = true;
        uint32_t idInScene = ~0u;
    };

    class ElecNekoScene
    {
    public:
    };
} // namespace ElecNeko
