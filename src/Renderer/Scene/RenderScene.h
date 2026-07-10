// src/Renderer/Scene/RenderScene.h
#pragma once

#include "Loader/Material.h"

#include "Math/Matrix.h"

#include "RHI2/RHIBuffer.h"
#include "RHI2/RHITexture.h"

#include "Renderer/Mesh/MeshDrawList.h"
#include "Renderer/Mesh/StaticMesh.h"

#include <cstdint>
#include <memory>
#include <vector>

namespace RHI
{
    class Device;
    class UploadBatch;
} // namespace RHI

namespace ElecNeko
{
    struct MeshInstance
    {
        uint32_t meshIndex = UINT32_MAX;

        Mat4 localToWorld;
        Mat4 worldToLocal;

        std::vector<uint32_t> materialOverrides;

        bool visible = true;
    };

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

        std::unique_ptr<RHI::Texture> textureArrayRHI;

        MeshDrawList drawList;

        std::vector<MeshInstanceGPU> gpuInstances;
        std::vector<Material_t> gpuMaterials;

        std::vector<Material> materials;

        std::unique_ptr<RHI::Buffer> instanceBufferRHI;
        std::unique_ptr<RHI::Buffer> materialBufferRHI;

        uint32_t AddMesh(std::unique_ptr<StaticMeshGPU> mesh);
        uint32_t AddMeshInstance(const MeshInstance &instance);

        void BuildDrawLists();
        void ClearDrawLists();

        bool UploadGpuSceneBuffers(RHI::Device *rhiDevice);

        RHI::Buffer *GetInstanceBuffer();
        RHI::Buffer *GetMaterialBuffer();

        const RHI::Buffer *GetInstanceBuffer() const;
        const RHI::Buffer *GetMaterialBuffer() const;

        uint64_t GetInstanceBufferSize() const;
        uint64_t GetMaterialBufferSize() const;

        bool CreateDefaultTextureArray(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch = nullptr);
        bool CreateTextureArrayFromRGBA8Pixels(RHI::Device *rhiDevice, RHI::UploadBatch *uploadBatch, uint32_t width, uint32_t height, uint32_t layers,
                                               const void *rgba8Pixels, uint64_t rgba8ByteSize, const char *debugName);
        RHI::Texture *GetTextureArray();
        const RHI::Texture *GetTextureArray() const;

        bool HasTextureArray() const;

        void Cleanup();
    };
} // namespace ElecNeko
