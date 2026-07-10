// src/Renderer/Scene/RenderScene.cpp
#include "Renderer/Scene/RenderScene.h"

#include "RHI2/RHIDevice.h"

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

                draw.vertexBuffer = mesh->vertexBufferRHI.get();
                draw.indexBuffer = mesh->indexBufferRHI.get();

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

    RHI::Buffer *RenderScene::GetInstanceBuffer() { return instanceBufferRHI.get(); }

    RHI::Buffer *RenderScene::GetMaterialBuffer() { return materialBufferRHI.get(); }

    const RHI::Buffer *RenderScene::GetInstanceBuffer() const { return instanceBufferRHI.get(); }

    const RHI::Buffer *RenderScene::GetMaterialBuffer() const { return materialBufferRHI.get(); }

    uint64_t RenderScene::GetInstanceBufferSize() const
    {
        if (!instanceBufferRHI)
        {
            return 0;
        }

        return instanceBufferRHI->GetSize();
    }

    uint64_t RenderScene::GetMaterialBufferSize() const
    {
        if (!materialBufferRHI)
        {
            return 0;
        }

        return materialBufferRHI->GetSize();
    }

    bool RenderScene::UploadGpuSceneBuffers(RHI::Device *rhiDevice)
    {
        if (rhiDevice == nullptr)
        {
            return false;
        }

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

        instanceBufferRHI.reset();
        materialBufferRHI.reset();

        if (!gpuInstances.empty())
        {
            const uint64_t instanceBufferSize = static_cast<uint64_t>(gpuInstances.size() * sizeof(gpuInstances[0]));

            RHI::BufferDesc instanceDesc{};
            instanceDesc.size = instanceBufferSize;
            instanceDesc.usage = RHI::BufferUsage::Storage | RHI::BufferUsage::TransferDst;
            instanceDesc.cpuVisible = true;
            instanceDesc.debugName = "RenderScene.InstanceBuffer";

            instanceBufferRHI = rhiDevice->CreateBuffer(instanceDesc, gpuInstances.data());

            if (!instanceBufferRHI)
            {
                printf("[RenderScene] Failed to create RHI2 instance buffer\n");
                return false;
            }
        }

        if (!gpuMaterials.empty())
        {
            const uint64_t materialBufferSize = static_cast<uint64_t>(gpuMaterials.size() * sizeof(gpuMaterials[0]));

            RHI::BufferDesc materialDesc{};
            materialDesc.size = materialBufferSize;
            materialDesc.usage = RHI::BufferUsage::Storage | RHI::BufferUsage::TransferDst;
            materialDesc.cpuVisible = true;
            materialDesc.debugName = "RenderScene.MaterialBuffer";

            materialBufferRHI = rhiDevice->CreateBuffer(materialDesc, gpuMaterials.data());

            if (!materialBufferRHI)
            {
                printf("[RenderScene] Failed to create RHI2 material buffer\n");
                return false;
            }
        }

        printf("[RenderScene] RHI2 scene buffers uploaded instances=%zu materials=%zu instanceBuffer=%llu materialBuffer=%llu\n", gpuInstances.size(),
               gpuMaterials.size(), static_cast<unsigned long long>(GetInstanceBufferSize()), static_cast<unsigned long long>(GetMaterialBufferSize()));

        return true;
    }

    RHI::Texture *RenderScene::GetTextureArray() { return textureArrayRHI.get(); }

    const RHI::Texture *RenderScene::GetTextureArray() const { return textureArrayRHI.get(); }

    bool RenderScene::HasTextureArray() const { return textureArrayRHI != nullptr; }

    bool RenderScene::CreateDefaultTextureArray(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch)
    {
        const uint8_t whitePixel[4] = {255, 255, 255, 255};

        const bool ok =
                CreateTextureArrayFromRGBA8Pixels(rhiDevice, uploadBatch, 1, 1, 1, whitePixel, sizeof(whitePixel), "RenderScene.DefaultWhiteTextureArray");

        if (ok)
        {
            printf("[RenderSceneTextureArray] created default white texture array\n");
        }

        return ok;
    }

    bool RenderScene::CreateTextureArrayFromRGBA8Pixels(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch, uint32_t width, uint32_t height, uint32_t layers,
                                                        const void *rgba8Pixels, uint64_t rgba8ByteSize, const char *debugName)
    {
        if (rhiDevice == nullptr || rgba8Pixels == nullptr || rgba8ByteSize == 0 || width == 0 || height == 0 || layers == 0)
        {
            return false;
        }

        RHI::TextureDesc desc{};
        desc.width = width;
        desc.height = height;
        desc.layers = layers;
        desc.format = RHI::Format::RGBA8_UNorm;
        desc.usage = RHI::TextureUsage::Sampled;
        desc.force2DArrayView = true;
        desc.debugName = debugName;

        textureArrayRHI = rhiDevice->CreateTexture(desc, rgba8Pixels, rgba8ByteSize, uploadBatch);

        if (!textureArrayRHI)
        {
            return false;
        }

        return true;
    }

    void RenderScene::Cleanup()
    {
        for (std::unique_ptr<StaticMeshGPU> &mesh: meshes)
        {
            if (mesh)
            {
                mesh->Cleanup();
            }
        }

        instanceBufferRHI.reset();
        materialBufferRHI.reset();

        textureArrayRHI.reset();

        meshes.clear();
        meshInstances.clear();

        materials.clear();

        gpuInstances.clear();
        gpuMaterials.clear();

        ClearDrawLists();
    }
} // namespace ElecNeko
