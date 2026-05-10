// src/Renderer/Scene/LegacyWorldConverter.cpp
#include "Renderer/Scene/LegacyWorldConverter.h"

#include "Renderer/Mesh/MeshVertex.h"
#include "Renderer/Mesh/StaticMesh.h"
#include "Renderer/Scene/RenderScene.h"


#include "RHI/DeviceContext.h"
#include "Scene/World.h"


#include <cassert>
#include <cstdint>
#include <memory>

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

        // 旧 VVertex 没有 tangent。
        // 这里先给默认值，后面做 normal map / PBR 时再从 loader 里正确生成 tangent。
        dst.tangent[0] = 1.0f;
        dst.tangent[1] = 0.0f;
        dst.tangent[2] = 0.0f;
        dst.tangent[3] = 1.0f;

        return dst;
    }

    static std::unique_ptr<StaticMeshGPU> ConvertLegacyMeshToStaticMeshGPU(DeviceContext *device, const ElecNekoMesh *legacyMesh)
    {
        assert(device != nullptr);
        assert(legacyMesh != nullptr);

        StaticMeshAsset asset;
        asset.name = legacyMesh->name;

        asset.vertices.reserve(legacyMesh->m_vertices.size());
        for (const VVertex &legacyVertex: legacyMesh->m_vertices)
        {
            asset.vertices.push_back(ConvertLegacyVertexToMeshVertex(legacyVertex));
        }

        asset.indices = legacyMesh->m_indices;

        // 旧 World 里的 ElecNekoMesh 本身没有 section/submesh 概念。
        // 旧系统是通过 ElecNekoMeshInstance::materialId 给整个 mesh 指定材质。
        //
        // 所以这里先生成一个 section，materialIndex 暂时写 0。
        // 真正使用哪个材质，在 MeshInstance::materialOverrides[0] 里覆盖。
        if (!asset.indices.empty())
        {
            StaticMeshSection section{};
            section.firstIndex = 0;
            section.indexCount = static_cast<uint32_t>(asset.indices.size());
            section.vertexOffset = 0;
            section.materialIndex = 0;

            asset.sections.push_back(section);
        }

        std::unique_ptr<StaticMeshGPU> meshGPU = std::make_unique<StaticMeshGPU>();

        const bool uploadOk = meshGPU->Upload(device, asset);
        assert(uploadOk);

        if (!uploadOk)
        {
            return nullptr;
        }

        return meshGPU;
    }

    std::unique_ptr<RenderScene> ConvertLegacyWorldToRenderScene(DeviceContext *device, World *world)
    {
        assert(device != nullptr);
        assert(world != nullptr);

        std::unique_ptr<RenderScene> renderScene = std::make_unique<RenderScene>();

        // 1. 复制材质数据。
        // Material 本身目前是值类型，直接复制即可。
        // renderScene->materials = world->m_materials;

        // // 2. 复用旧 Texture 指针。
        // // 注意：这里 RenderScene 不拥有 Texture，只是临时引用。
        // // Texture 的生命周期仍然由旧 World 管理。
        // renderScene->textures = world->m_textures;

        // if (world->defaultAlbedo != nullptr)
        // {
        //     renderScene->textures.push_back(world->defaultAlbedo);
        // }

        // if (world->defaultNormal != nullptr)
        // {
        //     renderScene->textures.push_back(world->defaultNormal);
        // }

        // if (world->defaultMetalRough != nullptr)
        // {
        //     renderScene->textures.push_back(world->defaultMetalRough);
        // }

        // if (world->defaultEmission != nullptr)
        // {
        //     renderScene->textures.push_back(world->defaultEmission);
        // }
        renderScene->materials = world->m_materials;

        // 纹理数组顺序必须先和旧 Scene 保持一致：
        // [0 ... world->m_textures.size() - 1] = 真实材质纹理
        // [world->m_textures.size()] = defaultAlbedo
        //
        // 这样 Material 里的 baseColorTexId / normalTexId / metalRoughTexId
        // 暂时不会因为 texture array 顺序变化而错位。
        renderScene->textures.clear();
        renderScene->textures.reserve(world->m_textures.size() + 1);

        for (Texture *texture: world->m_textures)
        {
            renderScene->textures.push_back(texture);
        }

        if (world->defaultAlbedo != nullptr)
        {
            renderScene->textures.push_back(world->defaultAlbedo);
        }

        // 3. 转换 mesh。
        // 旧 World 里每个 ElecNekoMesh 已经 MakeVBO 过了，
        // 但新系统需要自己的 StaticMeshGPU，所以这里会重新上传一份。
        //
        // 这只是过渡阶段。
        // 后面真正重写 loader 后，就不会重复上传。
        renderScene->meshes.reserve(world->m_meshes.size());

        for (const ElecNekoMesh *legacyMesh: world->m_meshes)
        {
            if (legacyMesh == nullptr)
            {
                continue;
            }

            std::unique_ptr<StaticMeshGPU> newMesh = ConvertLegacyMeshToStaticMeshGPU(device, legacyMesh);

            if (newMesh)
            {
                renderScene->AddMesh(std::move(newMesh));
            }
        }

        // 4. 转换 instance。
        renderScene->meshInstances.reserve(world->m_meshInstances.size());

        for (const ElecNekoMeshInstance &legacyInstance: world->m_meshInstances)
        {
            if (legacyInstance.meshId < 0)
            {
                continue;
            }

            if (static_cast<size_t>(legacyInstance.meshId) >= renderScene->meshes.size())
            {
                continue;
            }

            MeshInstance instance{};
            instance.meshIndex = static_cast<uint32_t>(legacyInstance.meshId);
            instance.localToWorld = legacyInstance.transform;
            instance.worldToLocal = legacyInstance.transform.Inverse();
            instance.visible = true;

            // 旧系统中，一个 instance 对应一个 materialId。
            // 新系统中，一个 mesh 可以有多个 section，所以这里先覆盖 section 0。
            if (legacyInstance.materialId >= 0)
            {
                instance.materialOverrides.push_back(static_cast<uint32_t>(legacyInstance.materialId));
            }

            renderScene->AddMeshInstance(instance);
        }

        // 5. 构建 draw list。
        // renderScene->BuildDrawLists();

        // // 6. 上传新 RenderScene 自己的 GPU 场景数据。
        // // 这包括：
        // // - instanceBuffer：每个实例的 localToWorld / worldToLocal
        // // - materialBuffer：每个材质的 Material_t
        // const bool uploadSceneBuffersOk = renderScene->UploadGpuSceneBuffers(device);
        // assert(uploadSceneBuffersOk);

        // return renderScene;
        renderScene->BuildDrawLists();

        const bool createTextureArrayOk = renderScene->CreateTextureArray(device);
        assert(createTextureArrayOk);

        const bool uploadSceneBuffersOk = renderScene->UploadGpuSceneBuffers(device);
        assert(uploadSceneBuffersOk);

        printf("[RenderSceneTextureArray] textures=%zu textureArray=%p\n", renderScene->textures.size(), static_cast<void *>(renderScene->textureArray));

        return renderScene;
    }
} // namespace ElecNeko
