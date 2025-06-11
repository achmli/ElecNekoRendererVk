#define TINYOBJLOADER_IMPLEMENTATION

#include "Mesh.h"

#include <unordered_map>
#include <cassert>

#include "tiny_obj_loader.h"

namespace ElecNeko
{
    bool MeshPart::MakeVBO(DeviceContext* device) 
    { 
        // VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[0];
        if (m_vertices.empty())
            return true;

        int bufferSize = static_cast<int>(sizeof(VVertex) * m_vertices.size());
        if (!m_vertexBuffer.Allocate(device, m_vertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
        {
            printf("Failed to allocate vertex buffer!\n");
            assert(0);
            return false;
        }

        bufferSize = static_cast<int>(sizeof(uint32_t) * m_indices.size());
        if (!m_indexBuffer.Allocate(device, m_indices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
        {
            printf("Failed to allocate index buffer!\n");
            assert(0);
            return false;
        }

        int hasTex[2] = {0, 0};

        if (albTexIndex > -1)
        {
            hasTex[0] = 1;
        }

        if (norTexIndex > -1)
        {
            hasTex[1] = 1;
        }
        
        bufferSize = static_cast<int>(sizeof(int) * 2);
        if (!m_uniformBuffer.Allocate(device, hasTex, bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        {
            printf("Failed to allocate mesh part's uniform buffer!\n");
            assert(0);
            return false;
        }

        m_isVBO = true;
        return true;
    }

    void MeshPart::Cleanup(DeviceContext *device)
    {
        if (!m_isVBO)
        {
            return;
        }

        m_vertexBuffer.Cleanup(device);
        m_indexBuffer.Cleanup(device);
        m_uniformBuffer.Cleanup(device);
    }

    void MeshPart::DrawIndexed(VkCommandBuffer vkCommandBuffer)
    {
        // bind the model
        VkBuffer vertexBuffers[] = {m_vertexBuffer.m_vkBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, m_indexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);

        // issue draw command
        vkCmdDrawIndexed(vkCommandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);
    }

    bool Mesh::LoadFromFile(DeviceContext *device, const std::string &name)
    { 
        std::string inputFile = "../res/models/" + name + "/" + name + ".obj";
        tinyobj::ObjReaderConfig readerConfig;
        readerConfig.mtl_search_path = "../res/models/" + name + "/";

        tinyobj::ObjReader reader;

        if (!reader.ParseFromFile(inputFile, readerConfig))
        {
            if (!reader.Error().empty())
            {
                printf("TinyObjReader: %s \n", reader.Error().c_str());
            }
            return false;
        }

        if (!reader.Warning().empty())
        {
            printf("TinyObjReader: %s \n", reader.Warning().c_str());
        }

        auto &attrib = reader.GetAttrib();
        auto &shapes = reader.GetShapes();
        auto &materials = reader.GetMaterials();

        std::string texPath = readerConfig.mtl_search_path;

        m_meshParts.resize(materials.size() + 1);

        std::unordered_map<std::string, int32_t> matAlbTex;
        std::unordered_map<std::string, int32_t> matNorTex;
        
        for (size_t i = 0; i < materials.size(); i++)
        {
            // read albedo map
            if (materials[i].diffuse_texname != "")
            {
                // if albedo map has been loaded
                if (matAlbTex.find(materials[i].diffuse_texname) != matAlbTex.end())
                {
                    m_meshParts[i + 1].albTexIndex = matAlbTex[materials[i].diffuse_texname];
                }
                else
                {
                    m_meshParts[i + 1].albTexIndex = albedoMaps.size();
                    matAlbTex[materials[i].diffuse_texname] = albedoMaps.size();

                    albedoMaps.emplace_back();
                    albedoMaps.back().LoadTexture(device, texPath + materials[i].diffuse_texname);
                }
            }
            //read normal map
            if (materials[i].normal_texname != "")
            {
                // normalMaps[i].LoadTexture(device, texPath + materials[i].normal_texname);
                if (matNorTex.find(materials[i].normal_texname) != matNorTex.end())
                {
                    m_meshParts[i + 1].norTexIndex = matNorTex[materials[i].normal_texname];
                }
                else
                {
                    m_meshParts[i + 1].norTexIndex = normalMaps.size();
                    matNorTex[materials[i].normal_texname] = normalMaps.size();

                    normalMaps.emplace_back();
                    normalMaps.back().LoadTexture(device, texPath + materials[i].normal_texname);
                }
            }
            else if (materials[i].bump_texname != "")
            {
                if (matNorTex.find(materials[i].bump_texname) != matNorTex.end())
                {
                    m_meshParts[i + 1].norTexIndex = matNorTex[materials[i].bump_texname];
                }
                else
                {
                    m_meshParts[i + 1].norTexIndex = normalMaps.size();
                    matNorTex[materials[i].bump_texname] = normalMaps.size();

                    normalMaps.emplace_back();
                    normalMaps.back().LoadTexture(device, texPath + materials[i].bump_texname);
                }
            }
        }

        std::vector<std::unordered_map<VVertex, uint32_t, VVertexHash>> mapVec;
        mapVec.resize(materials.size() + 1);

        for (const auto& shape : shapes)
        {
            size_t indexOffset = 0;
            for (size_t n = 0; n < shape.mesh.num_face_vertices.size(); n++)
            {
                auto ngon = shape.mesh.num_face_vertices[n];
                auto material_id = shape.mesh.material_ids[n];

                size_t partIndex = static_cast<size_t>(material_id + 1);

                std::vector<VVertex> faceVerts;

                for (size_t f = 0; f < ngon; f++)
                {
                    const auto &index = shape.mesh.indices[indexOffset + f];

                    VVertex vertex;

                    vertex.position[0] = attrib.vertices[3 * index.vertex_index + 0];
                    vertex.position[1] = attrib.vertices[3 * index.vertex_index + 1];
                    vertex.position[2] = attrib.vertices[3 * index.vertex_index + 2];

                    if (index.texcoord_index >= 0)
                    {
                        vertex.uv[0] = attrib.texcoords[2 * index.texcoord_index + 0];
                        vertex.uv[1] = 1.f - attrib.texcoords[2 * index.texcoord_index + 1];
                    }
                    else
                    {
                        vertex.uv[0] = vertex.uv[1] = 0.f;
                    }
                    if (index.normal_index >= 0)
                    {
                        vertex.normal[0] = attrib.normals[3 * index.normal_index + 0];
                        vertex.normal[1] = attrib.normals[3 * index.normal_index + 1];
                        vertex.normal[2] = attrib.normals[3 * index.normal_index + 2];
                    }
                    else
                    {
                        vertex.normal[0] = 0.f;
                        vertex.normal[1] = 0.f;
                        vertex.normal[2] = 1.f;
                    }

                    // m_meshParts[material_id + 1].m_vertices.emplace_back(vertex);
                    // m_meshParts[material_id + 1].m_indices.emplace_back(m_meshParts[material_id + 1].m_indices.size());
                    faceVerts.push_back(vertex);
                }

                // triangulate non-triangle face
                for (int v = 1; v < ngon - 1; v++)
                {
                    VVertex triVerts[3] = {faceVerts[0], faceVerts[v], faceVerts[v + 1]};

                    for (int k = 0; k < 3; k++)
                    {
                        VVertex &vv = triVerts[k];
                        auto &mpMap = mapVec[partIndex];

                        if (mpMap.find(vv) == mpMap.end())
                        {
                            uint32_t newIndex = static_cast<uint32_t>(m_meshParts[partIndex].m_vertices.size());
                            mpMap[vv] = newIndex;
                            m_meshParts[partIndex].m_vertices.push_back(vv);
                            m_meshParts[partIndex].m_indices.push_back(newIndex);
                        }
                        else
                        {
                            m_meshParts[partIndex].m_indices.push_back(mpMap[vv]);
                        }
                    }
                }

                indexOffset += ngon;
            }
        }
        return true;
    }

    bool Mesh::MakeUBO(DeviceContext *device)
    {
        Vec3 fwd = Vec3(1, 0, 0);
        Vec3 up = Vec3(0, 0, 1);

        Mat4 matOrient;
        matOrient.Orient(pos, fwd, up);

        Mat4 S = Mat4::Scaling(scale);
        matOrient = matOrient * S;

        matOrient = matOrient.Transpose();

        int bufferSize = sizeof(matOrient);
        if (!uniformBuffer.Allocate(device, matOrient.ToPtr(), bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT))
        {
            printf("Failed to allocate uniform buffer!\n");
            assert(0);
            return false;
        }

        isUBO = true;
        return true;
    }

    void Mesh::Cleanup(DeviceContext* device)
    {
        for (auto& meshparts : m_meshParts)
        {
            meshparts.Cleanup(device);
        }

        if (!isUBO)
        {
            return;
        }

        uniformBuffer.Cleanup(device);

        for (auto& map : albedoMaps)
        {
            map.Cleanup(device);
        }
        albedoMaps.clear();
        
        for (auto& map : normalMaps)
        {
            map.Cleanup(device);
        }
        normalMaps.clear();
    }
}


