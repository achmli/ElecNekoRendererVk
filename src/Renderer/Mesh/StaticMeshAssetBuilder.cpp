// src/Renderer/Mesh/StaticMeshAssetBuilder.cpp
#include "Renderer/Mesh/StaticMeshAssetBuilder.h"

#include "Scene/World.h"

#include <cstdint>

namespace ElecNeko
{
    static MeshVertex ConvertLegacyVertexToMeshVertex(const VVertex &src)
    {
        MeshVertex dst{};

        dst.position[0] = src.position[0];
        dst.position[1] = src.position[1];
        dst.position[2] = src.position[2];

        dst.uv[0] = src.uv[0];
        dst.uv[1] = src.uv[1];

        dst.normal[0] = src.normal[0];
        dst.normal[1] = src.normal[1];
        dst.normal[2] = src.normal[2];

        dst.tangent[0] = 1.0f;
        dst.tangent[1] = 0.0f;
        dst.tangent[2] = 0.0f;
        dst.tangent[3] = 1.0f;

        return dst;
    }

    StaticMeshAsset StaticMeshAssetBuilder::BuildFromLegacyMesh(const ElecNekoMesh &legacyMesh)
    {
        StaticMeshAsset asset;
        asset.name = legacyMesh.name;

        asset.vertices.reserve(legacyMesh.m_vertices.size());

        for (const VVertex &legacyVertex: legacyMesh.m_vertices)
        {
            asset.vertices.push_back(ConvertLegacyVertexToMeshVertex(legacyVertex));
        }

        asset.indices = legacyMesh.m_indices;

        if (!asset.indices.empty())
        {
            StaticMeshSection section{};
            section.firstIndex = 0;
            section.indexCount = static_cast<uint32_t>(asset.indices.size());
            section.vertexOffset = 0;
            section.materialIndex = 0;

            asset.sections.push_back(section);
        }

        return asset;
    }
} // namespace ElecNeko
