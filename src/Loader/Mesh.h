#pragma once

#include "Texture.h"

#include "Math/Vector.h"
#include "Math/Quat.h"

#include <vector>
#include <array>

namespace ElecNeko
{
    struct VVertex
    {
        float position[3];
        float uv[2];
        float normal[3];

        static VkVertexInputBindingDescription GetBindingDescription()
        {
            VkVertexInputBindingDescription bindingDescription{};
            bindingDescription.binding = 0;
            bindingDescription.stride = sizeof(VVertex);
            bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

            return bindingDescription;
        }

        static std::array<VkVertexInputAttributeDescription, 3> GetAttributeDescriptions()
        {
            std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};

            attributeDescriptions[0].binding = 0;
            attributeDescriptions[0].location = 0;
            attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[0].offset = offsetof(VVertex, position);

            attributeDescriptions[1].binding = 0;
            attributeDescriptions[1].location = 1;
            attributeDescriptions[1].format = VK_FORMAT_R32G32_SFLOAT;
            attributeDescriptions[1].offset = offsetof(VVertex, uv);

            attributeDescriptions[2].binding = 0;
            attributeDescriptions[2].location = 2;
            attributeDescriptions[2].format = VK_FORMAT_R32G32B32_SFLOAT;
            attributeDescriptions[2].offset = offsetof(VVertex, normal);

            return attributeDescriptions;
        }
    };

    class MeshPart
    {
    public:
        MeshPart() : m_isVBO(false) {}
        ~MeshPart() = default;

        bool MakeVBO(DeviceContext *device);

        void Cleanup(DeviceContext *device);

        void DrawIndexed(VkCommandBuffer vkCommandBUffer);

    public:
        std::vector<VVertex> m_vertices;
        std::vector<uint32_t> m_indices;

        uint32_t texMapIndex = -1;

        Buffer m_vertexBuffer;
        Buffer m_indexBuffer;

        bool m_isVBO;
    };

    class Mesh
    {
    public:
        Mesh() : pos(Vec3(0.f, 0.f, 0.f)), rot(Vec3(1.f, 0.f, 0.f), 3.14f / 2.f), scale(Vec3(1.f, 1.f, 1.f)), isUBO(false) {}
        Mesh(const Vec3 &Pos, const Quat &Rot, const Vec3 &Scale) : pos(Pos), rot(Rot), scale(Scale), isUBO(false) {}
        ~Mesh() = default;

        bool LoadFromFile(DeviceContext *device, const std::string &name);
        bool MakeUBO(DeviceContext *device);

        void Cleanup(DeviceContext *device);
    public:
        std::vector<MeshPart> m_meshParts;
        std::vector<Texture> albedoMaps;
        std::vector<Texture> normalMaps;
        Buffer uniformBuffer;

        Vec3 pos;
        Quat rot;
        Vec3 scale;
        bool isUBO;
    };
}
