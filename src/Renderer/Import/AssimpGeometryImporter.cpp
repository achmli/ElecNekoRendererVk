// src/Renderer/Import/AssimpGeometryImporter.cpp
#include "Renderer/Import/AssimpGeometryImporter.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <cstdint>
#include <cstring>
#include <filesystem>
#include <functional>
#include <string>
#include <unordered_map>

namespace ElecNeko
{
    namespace
    {
        struct MeshVertexKey
        {
            MeshVertex vertex;

            bool operator==(const MeshVertexKey &rhs) const { return vertex == rhs.vertex; }
        };

        struct MeshVertexKeyHash
        {
            size_t operator()(const MeshVertexKey &key) const noexcept
            {
                const MeshVertex &v = key.vertex;

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

        static void SetVec2(float dst[2], float x, float y)
        {
            dst[0] = x;
            dst[1] = y;
        }

        static void SetVec3(float dst[3], float x, float y, float z)
        {
            dst[0] = x;
            dst[1] = y;
            dst[2] = z;
        }

        static void SetVec4(float dst[4], float x, float y, float z, float w)
        {
            dst[0] = x;
            dst[1] = y;
            dst[2] = z;
            dst[3] = w;
        }

        static MeshVertex BuildVertexFromAiMesh(const aiMesh *mesh, uint32_t vertexIndex)
        {
            MeshVertex vertex{};

            if (mesh->HasPositions())
            {
                SetVec3(vertex.position, mesh->mVertices[vertexIndex].x, mesh->mVertices[vertexIndex].y, mesh->mVertices[vertexIndex].z);
            }
            else
            {
                SetVec3(vertex.position, 0.0f, 0.0f, 0.0f);
            }

            if (mesh->HasTextureCoords(0))
            {
                SetVec2(vertex.uv, mesh->mTextureCoords[0][vertexIndex].x, mesh->mTextureCoords[0][vertexIndex].y);
            }
            else
            {
                SetVec2(vertex.uv, 0.0f, 0.0f);
            }

            if (mesh->HasNormals())
            {
                SetVec3(vertex.normal, mesh->mNormals[vertexIndex].x, mesh->mNormals[vertexIndex].y, mesh->mNormals[vertexIndex].z);
            }
            else
            {
                SetVec3(vertex.normal, 0.0f, 0.0f, 1.0f);
            }

            if (mesh->HasTangentsAndBitangents())
            {
                SetVec4(vertex.tangent, mesh->mTangents[vertexIndex].x, mesh->mTangents[vertexIndex].y, mesh->mTangents[vertexIndex].z, 1.0f);
            }
            else
            {
                SetVec4(vertex.tangent, 1.0f, 0.0f, 0.0f, 1.0f);
            }

            return vertex;
        }

        static unsigned int BuildAssimpFlags(const GeometryImportOptions &options)
        {
            unsigned int flags = 0;

            if (options.triangulate)
            {
                flags |= aiProcess_Triangulate;
            }

            if (options.generateNormals)
            {
                flags |= aiProcess_GenSmoothNormals;
            }

            if (options.generateTangents)
            {
                flags |= aiProcess_CalcTangentSpace;
            }

            if (options.flipUVs)
            {
                flags |= aiProcess_FlipUVs;
            }

            flags |= aiProcess_JoinIdenticalVertices;
            flags |= aiProcess_ImproveCacheLocality;
            flags |= aiProcess_RemoveRedundantMaterials;
            flags |= aiProcess_FindInvalidData;

            return flags;
        }

        static void AppendMeshDeduplicated(const aiMesh *aiMesh, StaticMeshAsset &asset,
                                           std::unordered_map<MeshVertexKey, uint32_t, MeshVertexKeyHash> &vertexToIndex)
        {
            for (uint32_t faceIndex = 0; faceIndex < aiMesh->mNumFaces; ++faceIndex)
            {
                const aiFace &face = aiMesh->mFaces[faceIndex];

                if (face.mNumIndices != 3)
                {
                    continue;
                }

                for (uint32_t k = 0; k < 3; ++k)
                {
                    const uint32_t sourceVertexIndex = face.mIndices[k];

                    MeshVertexKey key{};
                    key.vertex = BuildVertexFromAiMesh(aiMesh, sourceVertexIndex);

                    auto it = vertexToIndex.find(key);

                    if (it == vertexToIndex.end())
                    {
                        const uint32_t newIndex = static_cast<uint32_t>(asset.vertices.size());
                        asset.vertices.push_back(key.vertex);
                        asset.indices.push_back(newIndex);
                        vertexToIndex.emplace(key, newIndex);
                    }
                    else
                    {
                        asset.indices.push_back(it->second);
                    }
                }
            }
        }

        static void AppendMeshWithoutDeduplication(const aiMesh *aiMesh, StaticMeshAsset &asset)
        {
            const uint32_t vertexOffset = static_cast<uint32_t>(asset.vertices.size());

            asset.vertices.reserve(asset.vertices.size() + aiMesh->mNumVertices);

            for (uint32_t vertexIndex = 0; vertexIndex < aiMesh->mNumVertices; ++vertexIndex)
            {
                asset.vertices.push_back(BuildVertexFromAiMesh(aiMesh, vertexIndex));
            }

            for (uint32_t faceIndex = 0; faceIndex < aiMesh->mNumFaces; ++faceIndex)
            {
                const aiFace &face = aiMesh->mFaces[faceIndex];

                if (face.mNumIndices != 3)
                {
                    continue;
                }

                asset.indices.push_back(vertexOffset + face.mIndices[0]);
                asset.indices.push_back(vertexOffset + face.mIndices[1]);
                asset.indices.push_back(vertexOffset + face.mIndices[2]);
            }
        }
    } // namespace

    GeometryImportResult AssimpGeometryImporter::ImportGeometryFile(const std::filesystem::path &filePath, const GeometryImportOptions &options)
    {
        GeometryImportResult result{};

        Assimp::Importer importer;

        const unsigned int flags = BuildAssimpFlags(options);

        const aiScene *scene = importer.ReadFile(filePath.string(), flags);

        if (scene == nullptr || scene->mNumMeshes == 0)
        {
            result.success = false;
            result.errorMessage = importer.GetErrorString();
            return result;
        }

        result.mesh.name = filePath.stem().string();

        if (options.deduplicateVertices)
        {
            std::unordered_map<MeshVertexKey, uint32_t, MeshVertexKeyHash> vertexToIndex;

            for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
            {
                const aiMesh *mesh = scene->mMeshes[meshIndex];

                if (mesh == nullptr)
                {
                    continue;
                }

                AppendMeshDeduplicated(mesh, result.mesh, vertexToIndex);
            }
        }
        else
        {
            for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
            {
                const aiMesh *mesh = scene->mMeshes[meshIndex];

                if (mesh == nullptr)
                {
                    continue;
                }

                AppendMeshWithoutDeduplication(mesh, result.mesh);
            }
        }

        if (!result.mesh.indices.empty())
        {
            StaticMeshSection section{};
            section.firstIndex = 0;
            section.indexCount = static_cast<uint32_t>(result.mesh.indices.size());
            section.vertexOffset = 0;

            // Geometry-only assets use one material slot.
            // The final material is assigned by the scene instance / material set.
            section.materialIndex = 0;

            result.mesh.sections.push_back(section);
        }

        result.success = !result.mesh.vertices.empty() && !result.mesh.indices.empty() && !result.mesh.sections.empty();

        if (!result.success)
        {
            result.errorMessage = "Imported geometry contains no valid triangles.";
        }

        return result;
    }
} // namespace ElecNeko
