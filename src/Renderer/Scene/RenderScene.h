// src/Renderer/Scene/RenderScene.h
#pragma once

#include "Loader/Material.h"
#include "Loader/Texture.h"

#include "Math/Matrix.h"

#include "RHI/Buffer.h"

#include "Renderer/Mesh/MeshDrawList.h"
#include "Renderer/Mesh/StaticMesh.h"

#include <cstdint>
#include <memory>
#include <vector>

class DeviceContext;

namespace ElecNeko
{
    struct MeshInstance
    {
        uint32_t meshIndex = UINT32_MAX;

        Mat4 localToWorld;
        Mat4 worldToLocal;

        // 为空时使用 StaticMeshSection::materialIndex。
        // 不为空时，按 sectionIndex 覆盖材质。
        std::vector<uint32_t> materialOverrides;

        bool visible = true;
    };

    // GPU 侧实例数据。
    // 后面 shader 里通过 instanceIndex 访问。
    struct MeshInstanceGPU
    {
        Mat4 localToWorld;
        Mat4 worldToLocal;
    };

    class RenderScene
    {
    public:
        std::vector<std::unique_ptr<StaticMeshGPU>> meshes;
        std::vector<MeshInstance> meshInstances;

        std::vector<Material> materials;
        std::vector<Texture *> textures;

        TextureArray *textureArray = nullptr;

        MeshDrawList drawList;

        // CPU 侧上传缓存。
        std::vector<MeshInstanceGPU> gpuInstances;
        std::vector<Material_t> gpuMaterials;

        // GPU buffer。
        // 暂时仍然用旧 Buffer 类，后面 RHI Device 完成后再替换成 RHI::BufferHandle 管理。
        Buffer instanceBuffer;
        Buffer materialBuffer;

        RHI::BufferHandle instanceBufferHandle;
        RHI::BufferHandle materialBufferHandle;

        uint32_t AddMesh(std::unique_ptr<StaticMeshGPU> mesh);
        uint32_t AddMeshInstance(const MeshInstance &instance);

        void BuildDrawLists();
        void ClearDrawLists();

        bool UploadGpuSceneBuffers(DeviceContext *device);

        bool CreateTextureArray(DeviceContext *device);

        void Cleanup(DeviceContext *device);
    };
} // namespace ElecNeko
