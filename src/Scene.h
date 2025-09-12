//
//  Scene.h
//
#pragma once
#include <vector>

#include "Physics/Body.h"
#include "Physics/Constraints.h"
#include "Physics/Manifold.h"
#include "Physics/Shapes.h"

#include "Scene/World.h"

namespace ElecNeko
{
    struct ElecNekoVertex
    {
        float position[3];
        float uv[2];
        float normal[3];
        uint32_t materialIdx;
        uint32_t modelMatrixIdx;

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(ElecNekoVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 5> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(ElecNekoVertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(ElecNekoVertex, uv);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(ElecNekoVertex, normal);

            attributeDescriptions[3].binding = 0;
            attributeDescriptions[3].location = 3;
            attributeDescriptions[3].format = VK_FORMAT_R32_UINT;
            attributeDescriptions[3].offset = offsetof(ElecNekoVertex, materialIdx);

            attributeDescriptions[4].binding = 0;
            attributeDescriptions[4].location = 4;
            attributeDescriptions[4].format = VK_FORMAT_R32_UINT;
            attributeDescriptions[4].offset = offsetof(ElecNekoVertex, modelMatrixIdx);

            return attributeDescriptions;
        }
    };
    class Scene
    {
    public:
        Scene() = default;
        ~Scene() = default;

        bool Initialize(DeviceContext *device, const std::string &sceneFile);
        bool Initialize(DeviceContext *device, World *inWorld);
        void Cleanup(DeviceContext *device);

        void DrawOpaqueIndexed(VkCommandBuffer vkCommandBuffer);
        void DrawMaskIndexed(VkCommandBuffer vkCommandBuffer);
        void DrawTransparentIndexed(VkCommandBuffer vkCommandBuffer);

        bool MakeVBO(DeviceContext *device);
        bool MakeUBO(DeviceContext *device);

    public:
        std::vector<ElecNekoVertex> opaqueVertices;
        std::vector<uint32_t> opaqueIndices;

        std::vector<ElecNekoVertex> maskVertices;
        std::vector<uint32_t> maskIndices;

        std::vector<ElecNekoVertex> transparentVertices;
        std::vector<uint32_t> transparentIndices;

        std::vector<Material_t> materials;
        std::vector<Mat4> modelMatrices;

        TextureArray *textureArray;

        World *world;

        Buffer opaqueVertexBuffer;
        Buffer opaqueIndexBuffer;

        Buffer maskVertexBuffer;
        Buffer maskIndexBuffer;

        Buffer transparentVertexBuffer;
        Buffer transparentIndexBuffer;

        Buffer materialBuffer;
        Buffer modelMatrixBuffer;
    };
} // namespace ElecNeko
