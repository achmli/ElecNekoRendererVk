// src/Renderer/Mesh/AssimpStaticMeshLoader.cpp
#include "Renderer/Mesh/AssimpStaticMeshLoader.h"

#include <assimp/mesh.h>
#include <cassert>
#include <cstdint>

namespace ElecNeko
{
    static void SetVec3(float dst[3], float x, float y, float z)
    {
        dst[0] = x;
        dst[1] = y;
        dst[2] = z;
    }

    static void SetVec2(float dst[2], float x, float y)
    {
        dst[0] = x;
        dst[1] = y;
    }

    static void SetVec4(float dst[4], float x, float y, float z, float w)
    {
        dst[0] = x;
        dst[1] = y;
        dst[2] = z;
        dst[3] = w;
    }

    StaticMeshAsset AssimpStaticMeshLoader::BuildStaticMeshAssetFromAiMesh(const aiMesh *mesh, const std::string &name, uint32_t materialIndex)
    {
        assert(mesh != nullptr);

        StaticMeshAsset asset;
        asset.name = name;

        if (mesh == nullptr)
        {
            return asset;
        }

        asset.vertices.reserve(mesh->mNumVertices);

        for (uint32_t i = 0; i < mesh->mNumVertices; ++i)
        {
            MeshVertex vertex{};

            if (mesh->HasPositions())
            {
                SetVec3(vertex.position, mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
            }
            else
            {
                SetVec3(vertex.position, 0.0f, 0.0f, 0.0f);
            }

            if (mesh->HasTextureCoords(0))
            {
                SetVec2(vertex.uv, mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);
            }
            else
            {
                SetVec2(vertex.uv, 0.0f, 0.0f);
            }

            if (mesh->HasNormals())
            {
                SetVec3(vertex.normal, mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z);
            }
            else
            {
                SetVec3(vertex.normal, 0.0f, 0.0f, 1.0f);
            }

            if (mesh->HasTangentsAndBitangents())
            {
                SetVec4(vertex.tangent, mesh->mTangents[i].x, mesh->mTangents[i].y, mesh->mTangents[i].z, 1.0f);
            }
            else
            {
                SetVec4(vertex.tangent, 1.0f, 0.0f, 0.0f, 1.0f);
            }

            asset.vertices.push_back(vertex);
        }

        asset.indices.reserve(mesh->mNumFaces * 3);

        for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex)
        {
            const aiFace &face = mesh->mFaces[faceIndex];

            if (face.mNumIndices != 3)
            {
                continue;
            }

            asset.indices.push_back(face.mIndices[0]);
            asset.indices.push_back(face.mIndices[1]);
            asset.indices.push_back(face.mIndices[2]);
        }

        if (!asset.indices.empty())
        {
            StaticMeshSection section{};
            section.firstIndex = 0;
            section.indexCount = static_cast<uint32_t>(asset.indices.size());
            section.vertexOffset = 0;
            section.materialIndex = materialIndex;

            asset.sections.push_back(section);
        }

        return asset;
    }
} // namespace ElecNeko
