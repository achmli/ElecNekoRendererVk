//
//  Scene.cpp
//
#include "Scene.h"
#include "Physics/Broadphase.h"
#include "Physics/Contact.h"
#include "Physics/Intersections.h"

namespace ElecNeko
{
    bool Scene::Initialize(DeviceContext *device, const std::string &sceneFile)
    {
        world = new World();

        world->CreateDefaultTextures(device);
        if (!world->LoadSceneFromFile(device, sceneFile))
        {
            return false;
        }

        // textures.reserve(world->m_textures.size() + 1);
        // textures.insert(textures.end(), world->m_textures.begin(), world->m_textures.end());
        // textures.push_back(world->defaultAlbedo);
        std::vector<TextureProperty> properties;
        properties.reserve(world->m_textures.size() + 1);
        for (auto tex: world->m_textures)
        {
            properties.push_back(tex->ExtractProperties());
        }
        properties.push_back(world->defaultAlbedo->ExtractProperties());
        textureArray = new TextureArray();
        if (!textureArray->CreateFromData(device, properties, 2048, 2048, 4))
        {
            printf("Failed to create texture array!\n");
            assert(0);
            return false;
        }

        materials.clear();
        materials.reserve(world->m_materials.size());
        for (auto material: world->m_materials)
        {
            materials.push_back(material.MakeStrcut());
        }

        int opaqueIndexOffset = 0;
        int maskIndexOffset = 0;
        int transparentIndexOffset = 0;
        for (int i = 0; i < world->m_meshInstances.size(); i++)
        {
            int matID = world->m_meshInstances[i].materialId;
            int meshID = world->m_meshInstances[i].meshId;

            modelMatrices.push_back(world->m_meshInstances[i].transform);

            for (int j = 0; j < world->m_meshes[meshID]->m_indices.size(); j++)
            {
                uint32_t idx = world->m_meshes[meshID]->m_indices[j];
                if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
                {
                    idx += maskIndexOffset;
                    maskIndices.push_back(idx);
                }
                else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
                {
                    idx += transparentIndexOffset;
                    transparentIndices.push_back(idx);
                }
                else
                {
                    idx += opaqueIndexOffset;
                    opaqueIndices.push_back(idx);
                }
            }

            for (int j = 0; j < world->m_meshes[meshID]->m_vertices.size(); j++)
            {
                ElecNekoVertex v = {};
                v.position[0] = world->m_meshes[meshID]->m_vertices[j].position[0];
                v.position[1] = world->m_meshes[meshID]->m_vertices[j].position[1];
                v.position[2] = world->m_meshes[meshID]->m_vertices[j].position[2];

                v.normal[0] = world->m_meshes[meshID]->m_vertices[j].normal[0];
                v.normal[1] = world->m_meshes[meshID]->m_vertices[j].normal[1];
                v.normal[2] = world->m_meshes[meshID]->m_vertices[j].normal[2];

                v.uv[0] = world->m_meshes[meshID]->m_vertices[j].uv[0];
                v.uv[1] = world->m_meshes[meshID]->m_vertices[j].uv[1];

                v.materialIdx = matID;
                v.modelMatrixIdx = i;
                if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
                {
                    maskVertices.push_back(v);
                }
                else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
                {
                    transparentVertices.push_back(v);
                }
                else
                {
                    opaqueVertices.push_back(v);
                }
            }
            if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
            {
                maskIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
            else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
            {
                transparentIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
            else
            {
                opaqueIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
        }

        return true;
    }

    bool Scene::Initialize(DeviceContext *device, World *inWorld)
    {
        world = inWorld;
        std::vector<TextureProperty> properties;
        properties.reserve(world->m_textures.size() + 1);
        for (auto tex: world->m_textures)
        {
            properties.push_back(tex->ExtractProperties());
        }
        properties.push_back(world->defaultAlbedo->ExtractProperties());
        textureArray = new TextureArray();
        if (!textureArray->CreateFromData(device, properties, 2048, 2048, 4))
        {
            printf("Failed to create texture array!\n");
            assert(0);
            return false;
        }

        materials.clear();
        materials.reserve(world->m_materials.size());
        for (auto material: world->m_materials)
        {
            materials.push_back(material.MakeStrcut());
        }

        int opaqueIndexOffset = 0;
        int maskIndexOffset = 0;
        int transparentIndexOffset = 0;
        for (int i = 0; i < world->m_meshInstances.size(); i++)
        {
            int matID = world->m_meshInstances[i].materialId;
            int meshID = world->m_meshInstances[i].meshId;

            modelMatrices.push_back(world->m_meshInstances[i].transform);

            for (int j = 0; j < world->m_meshes[meshID]->m_indices.size(); j++)
            {
                uint32_t idx = world->m_meshes[meshID]->m_indices[j];
                if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
                {
                    idx += maskIndexOffset;
                    maskIndices.push_back(idx);
                }
                else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
                {
                    idx += transparentIndexOffset;
                    transparentIndices.push_back(idx);
                }
                else
                {
                    idx += opaqueIndexOffset;
                    opaqueIndices.push_back(idx);
                }
            }

            for (int j = 0; j < world->m_meshes[meshID]->m_vertices.size(); j++)
            {
                ElecNekoVertex v = {};
                v.position[0] = world->m_meshes[meshID]->m_vertices[j].position[0];
                v.position[1] = world->m_meshes[meshID]->m_vertices[j].position[1];
                v.position[2] = world->m_meshes[meshID]->m_vertices[j].position[2];

                v.normal[0] = world->m_meshes[meshID]->m_vertices[j].normal[0];
                v.normal[1] = world->m_meshes[meshID]->m_vertices[j].normal[1];
                v.normal[2] = world->m_meshes[meshID]->m_vertices[j].normal[2];

                v.uv[0] = world->m_meshes[meshID]->m_vertices[j].uv[0];
                v.uv[1] = world->m_meshes[meshID]->m_vertices[j].uv[1];

                v.materialIdx = matID;
                v.modelMatrixIdx = i;
                if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
                {
                    maskVertices.push_back(v);
                }
                else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
                {
                    transparentVertices.push_back(v);
                }
                else
                {
                    opaqueVertices.push_back(v);
                }
            }
            if (world->m_materials[matID].alphaMode == AlphaMode::Mask)
            {
                maskIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
            else if (world->m_materials[matID].alphaMode == AlphaMode::Blend)
            {
                transparentIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
            else
            {
                opaqueIndexOffset += world->m_meshes[meshID]->m_vertices.size();
            }
        }

        return true;
    }


    bool Scene::MakeVBO(DeviceContext *device)
    {
        if (!opaqueVertices.empty())
        {
            int bufferSize = static_cast<int>(sizeof(ElecNekoVertex) * opaqueVertices.size());
            if (!opaqueVertexBuffer.Allocate(device, opaqueVertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
            {
                printf("Failed to Allocate vertex Buffer!\n");
                assert(0);
                return false;
            }

            bufferSize = static_cast<int>(sizeof(uint32_t) * opaqueIndices.size());
            if (!opaqueIndexBuffer.Allocate(device, opaqueIndices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
            {
                printf("Failed to Allocate  indices Buffer!\n");
                assert(0);
                return false;
            }
        }

        if (!maskVertices.empty())
        {
            int bufferSize = static_cast<int>(sizeof(ElecNekoVertex) * maskVertices.size());
            if (!maskVertexBuffer.Allocate(device, maskVertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
            {
                printf("Failed to Allocate vertex Buffer!\n");
                assert(0);
                return false;
            }

            bufferSize = static_cast<int>(sizeof(uint32_t) * maskIndices.size());
            if (!maskIndexBuffer.Allocate(device, maskIndices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
            {
                printf("Failed to Allocate  indices Buffer!\n");
                assert(0);
                return false;
            }
        }

        if (!transparentVertices.empty())
        {
            int bufferSize = static_cast<int>(sizeof(ElecNekoVertex) * transparentVertices.size());
            if (!transparentVertexBuffer.Allocate(device, transparentVertices.data(), bufferSize, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT))
            {
                printf("Failed to Allocate vertex Buffer!\n");
                assert(0);
                return false;
            }

            bufferSize = static_cast<int>(sizeof(uint32_t) * transparentIndices.size());
            if (!transparentIndexBuffer.Allocate(device, transparentIndices.data(), bufferSize, VK_BUFFER_USAGE_INDEX_BUFFER_BIT))
            {
                printf("Failed to Allocate  indices Buffer!\n");
                assert(0);
                return false;
            }
        }

        return true;
    }

    bool Scene::MakeUBO(DeviceContext *device)
    {
        if (!materials.empty())
        {
            int bufferSize = static_cast<int>(sizeof(Material_t) * materials.size());
            if (!materialBuffer.Allocate(device, materials.data(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            {
                printf("Failed to allocate material buffer!\n");
                assert(0);
                return false;
            }
        }
        if (!modelMatrices.empty())
        {
            int bufferSize = static_cast<int>(sizeof(Mat4) * modelMatrices.size());
            if (!modelMatrixBuffer.Allocate(device, modelMatrices.data(), bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT))
            {
                printf("Failed to allocate model matrix buffer!\n");
                assert(0);
                return false;
            }
        }

        return true;
    }

    void Scene::Cleanup(DeviceContext *device)
    {
        if (world)
        {
            world = nullptr;
        }

        if (!opaqueVertices.empty())
        {
            opaqueVertexBuffer.Cleanup(device);
            opaqueIndexBuffer.Cleanup(device);
        }

        if (!maskVertices.empty())
        {
            maskVertexBuffer.Cleanup(device);
            maskIndexBuffer.Cleanup(device);
        }

        if (!transparentVertices.empty())
        {
            transparentVertexBuffer.Cleanup(device);
            transparentIndexBuffer.Cleanup(device);
        }

        if (!materials.empty())
        {
            materialBuffer.Cleanup(device);
        }
        if (!modelMatrices.empty())
        {
            modelMatrixBuffer.Cleanup(device);
        }

        textureArray->Cleanup(device);
    }

    void Scene::DrawOpaqueIndexed(VkCommandBuffer vkCommandBuffer)
    {
        VkBuffer vertexBuffers[] = {opaqueVertexBuffer.m_vkBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, opaqueIndexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(vkCommandBuffer, static_cast<uint32_t>(opaqueIndices.size()), 1, 0, 0, 0);
    }

    void Scene::DrawMaskIndexed(VkCommandBuffer vkCommandBuffer)
    {
        VkBuffer vertexBuffers[] = {maskVertexBuffer.m_vkBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, maskIndexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(vkCommandBuffer, static_cast<uint32_t>(maskIndices.size()), 1, 0, 0, 0);
    }

    void Scene::DrawTransparentIndexed(VkCommandBuffer vkCommandBuffer)
    {
        VkBuffer vertexBuffers[] = {transparentVertexBuffer.m_vkBuffer};
        VkDeviceSize offsets[] = {0};
        vkCmdBindVertexBuffers(vkCommandBuffer, 0, 1, vertexBuffers, offsets);
        vkCmdBindIndexBuffer(vkCommandBuffer, transparentIndexBuffer.m_vkBuffer, 0, VK_INDEX_TYPE_UINT32);

        vkCmdDrawIndexed(vkCommandBuffer, static_cast<uint32_t>(transparentIndices.size()), 1, 0, 0, 0);
    }

} // namespace ElecNeko
