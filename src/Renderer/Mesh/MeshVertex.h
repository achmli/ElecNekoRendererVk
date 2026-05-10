#pragma once

#include <cstdint>
#include <cstring>
#include <functional>

namespace ElecNeko
{
    // 新模型系统的统一顶点格式。
    // 注意：这里不要放 materialId / modelMatrixId。
    // 材质和实例索引以后通过 MeshDrawCommand / InstanceBuffer 传。
    struct MeshVertex
    {
        float position[3] = {0.0f, 0.0f, 0.0f};
        float uv[2] = {0.0f, 0.0f};
        float normal[3] = {0.0f, 0.0f, 1.0f};

        // 先预留 tangent，后面做 normal map / PBR 会用到。
        // w 通常用于 bitangent sign。
        float tangent[4] = {1.0f, 0.0f, 0.0f, 1.0f};

        bool operator==(const MeshVertex &other) const
        {
            return position[0] == other.position[0] && position[1] == other.position[1] && position[2] == other.position[2] && uv[0] == other.uv[0] &&
                   uv[1] == other.uv[1] && normal[0] == other.normal[0] && normal[1] == other.normal[1] && normal[2] == other.normal[2] &&
                   tangent[0] == other.tangent[0] && tangent[1] == other.tangent[1] && tangent[2] == other.tangent[2] && tangent[3] == other.tangent[3];
        }
    };

    struct MeshVertexHash
    {
        size_t operator()(const MeshVertex &v) const noexcept
        {
            size_t h = 0;

            auto hashFloat = [](float f)
            {
                uint32_t u = 0;
                std::memcpy(&u, &f, sizeof(float));
                return std::hash<uint32_t>()(u);
            };

            auto combine = [&](float f) { h ^= hashFloat(f) + 0x9e3779b9 + (h << 6) + (h >> 2); };

            combine(v.position[0]);
            combine(v.position[1]);
            combine(v.position[2]);

            combine(v.uv[0]);
            combine(v.uv[1]);

            combine(v.normal[0]);
            combine(v.normal[1]);
            combine(v.normal[2]);

            combine(v.tangent[0]);
            combine(v.tangent[1]);
            combine(v.tangent[2]);
            combine(v.tangent[3]);

            return h;
        }
    };

    // 临时兼容旧代码。
    // 后面把旧 VVertex 全部替换成 MeshVertex 后，再删掉这两个 using。
    // using VVertex = MeshVertex;
    // using VVertexHash = MeshVertexHash;
} // namespace ElecNeko
