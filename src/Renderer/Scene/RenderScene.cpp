// src/Renderer/Scene/RenderScene.cpp
#include "Renderer/Scene/RenderScene.h"

#include "RHI/DeviceContext.h"

#include <cassert>
#include <cstdio>

namespace ElecNeko
{
    uint32_t RenderScene::AddMesh(std::unique_ptr<StaticMeshGPU> mesh)
    {
        const uint32_t index = static_cast<uint32_t>(meshes.size());
        meshes.push_back(std::move(mesh));
        return index;
    }

    uint32_t RenderScene::AddMeshInstance(const MeshInstance &instance)
    {
        const uint32_t index = static_cast<uint32_t>(meshInstances.size());
        meshInstances.push_back(instance);
        return index;
    }

    void RenderScene::ClearDrawLists() { drawList.Clear(); }

    void RenderScene::BuildDrawLists()
    {
        ClearDrawLists();

        for (uint32_t instanceIndex = 0; instanceIndex < meshInstances.size(); ++instanceIndex)
        {
            const MeshInstance &instance = meshInstances[instanceIndex];

            if (!instance.visible)
            {
                continue;
            }

            if (instance.meshIndex >= meshes.size())
            {
                continue;
            }

            const StaticMeshGPU *mesh = meshes[instance.meshIndex].get();

            if (mesh == nullptr)
            {
                continue;
            }

            for (uint32_t sectionIndex = 0; sectionIndex < mesh->sections.size(); ++sectionIndex)
            {
                const StaticMeshSection &section = mesh->sections[sectionIndex];

                uint32_t materialIndex = section.materialIndex;

                if (sectionIndex < instance.materialOverrides.size())
                {
                    materialIndex = instance.materialOverrides[sectionIndex];
                }

                MeshDrawCommand draw{};
                draw.vertexBuffer = mesh->vertexBuffer;
                draw.indexBuffer = mesh->indexBuffer;
                draw.firstIndex = section.firstIndex;
                draw.indexCount = section.indexCount;
                draw.vertexOffset = section.vertexOffset;
                draw.materialIndex = materialIndex;
                draw.instanceIndex = instanceIndex;

                if (materialIndex < materials.size())
                {
                    const Material &material = materials[materialIndex];

                    if (material.alphaMode == AlphaMode::Mask)
                    {
                        draw.passType = MeshPassType::Masked;
                        drawList.masked.push_back(draw);
                    }
                    else if (material.alphaMode == AlphaMode::Blend)
                    {
                        draw.passType = MeshPassType::Transparent;
                        drawList.transparent.push_back(draw);
                    }
                    else
                    {
                        draw.passType = MeshPassType::Opaque;
                        drawList.opaque.push_back(draw);
                    }
                }
                else
                {
                    draw.passType = MeshPassType::Opaque;
                    drawList.opaque.push_back(draw);
                }

                MeshDrawCommand shadowDraw = draw;
                shadowDraw.passType = MeshPassType::Shadow;
                drawList.shadow.push_back(shadowDraw);
            }
        }
    }

    bool RenderScene::UploadGpuSceneBuffers(DeviceContext *device)
    {
        assert(device != nullptr);

        gpuInstances.clear();
        gpuInstances.reserve(meshInstances.size());

        for (const MeshInstance &instance: meshInstances)
        {
            MeshInstanceGPU gpuInstance{};
            gpuInstance.localToWorld = instance.localToWorld;
            gpuInstance.worldToLocal = instance.worldToLocal;
            gpuInstances.push_back(gpuInstance);
        }

        gpuMaterials.clear();
        gpuMaterials.reserve(materials.size());

        for (Material &material: materials)
        {
            gpuMaterials.push_back(material.MakeStrcut());
        }

        if (!gpuInstances.empty())
        {
            const int instanceBufferSize = static_cast<int>(sizeof(MeshInstanceGPU) * gpuInstances.size());

            const bool instanceOk = instanceBuffer.Allocate(device, gpuInstances.data(), instanceBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            assert(instanceOk);

            if (!instanceOk)
            {
                return false;
            }

            instanceBufferHandle = device->m_bufferRegistry.Register(&instanceBuffer);
        }

        if (!gpuMaterials.empty())
        {
            const int materialBufferSize = static_cast<int>(sizeof(Material_t) * gpuMaterials.size());

            const bool materialOk = materialBuffer.Allocate(device, gpuMaterials.data(), materialBufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT);

            assert(materialOk);

            if (!materialOk)
            {
                return false;
            }

            materialBufferHandle = device->m_bufferRegistry.Register(&materialBuffer);
        }

        return true;
    }

    bool RenderScene::CreateTextureArray(DeviceContext *device)
    {
        assert(device != nullptr);

        if (textureArray != nullptr)
        {
            textureArray->Cleanup(device);
            delete textureArray;
            textureArray = nullptr;
        }

        std::vector<TextureProperty> properties;
        properties.reserve(textures.size());

        for (Texture *texture: textures)
        {
            if (texture == nullptr)
            {
                continue;
            }

            properties.push_back(texture->ExtractProperties());
        }

        if (properties.empty())
        {
            return true;
        }

        textureArray = new TextureArray();

        if (!textureArray->CreateFromData(device, properties, 2048, 2048, 4, "render_scene_texture_array"))
        {
            printf("Failed to create RenderScene texture array!\n");
            delete textureArray;
            textureArray = nullptr;
            assert(false);
            return false;
        }

        return true;
    }

    void RenderScene::Cleanup(DeviceContext *device)
    {
        for (std::unique_ptr<StaticMeshGPU> &mesh: meshes)
        {
            if (mesh)
            {
                mesh->Cleanup(device);
            }
        }

        if (instanceBuffer.m_vkBuffer != VK_NULL_HANDLE)
        {
            instanceBuffer.Cleanup(device);
        }

        if (materialBuffer.m_vkBuffer != VK_NULL_HANDLE)
        {
            materialBuffer.Cleanup(device);
        }

        if (textureArray != nullptr)
        {
            textureArray->Cleanup(device);
            delete textureArray;
            textureArray = nullptr;
        }

        meshes.clear();
        meshInstances.clear();

        materials.clear();
        textures.clear();

        gpuInstances.clear();
        gpuMaterials.clear();

        instanceBufferHandle = {};
        materialBufferHandle = {};

        ClearDrawLists();
    }
} // namespace ElecNeko
