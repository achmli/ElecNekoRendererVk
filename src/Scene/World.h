#pragma once

#include "Light.h"
#include "Loader/Mesh.h"

#include "RHI/RHICommandList.h"

#include <unordered_map>

namespace ElecNeko
{
    class ElecNekoMesh
    {
    public:
        ElecNekoMesh() = default;
        ~ElecNekoMesh() = default;

        bool MakeVBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);

        // void DrawIndexed(VkCommandBuffer vkCommandBuffer);
        void DrawIndexed(RHICommandList &cmd);

    public:
        std::vector<VVertex> m_vertices;
        std::vector<uint32_t> m_indices;

        std::string name;

        Buffer m_vertexBuffer;
        Buffer m_indexBuffer;

        RHI::BufferHandle m_vertexBufferHandle;
        RHI::BufferHandle m_indexBufferHandle;
    };

    class ElecNekoMeshInstance
    {
    public:
        ElecNekoMeshInstance(const std::string &instanceName, const Mat4 &xform, int meshID, int materialID) :
            name(instanceName), transform(xform), meshId(meshID), materialId(materialID)
        {}
        ElecNekoMeshInstance() = delete;
        ~ElecNekoMeshInstance() = default;

        bool MakeUBO(DeviceContext *device);

        void Cleanup(DeviceContext *device);

    public:
        Mat4 transform;

        std::string name;

        int meshId;
        int materialId;

        Buffer uniformBuffer;
    };

    class World
    {
    public:
        World() : m_cam(nullptr), defaultAlbedo(nullptr), defaultNormal(nullptr), defaultMetalRough(nullptr), defaultEmission(nullptr) {}
        ~World() = default;

        int AddTexture(DeviceContext *device, const std::string &filename);
        int AddMaterial(const Material &material);
        // int AddMeshInstance(const ElecNekoMeshInstance &meshInstance);

        int LoadMeshGeometryOnly(DeviceContext *device, const std::string &filename);
        int LoadMeshWithMaterials(DeviceContext *device, const std::string &filename, Mat4 transMat);

        void AddCamera(Vec3 eye, Vec3 lookAt, float fov, float aspecRatio = (9.f / 16.f), float zNear = .1f, float zFar = 1000.f);

        int EnsureTextureCached(DeviceContext *device, const std::string &filename);

        bool LoadSceneFromFile(DeviceContext *device, const std::string &filename);

        void CreateDefaultTextures(DeviceContext *device);

        void UnloadScene(DeviceContext *device);

        void Cleanup(DeviceContext *device);
        void LightClean(DeviceContext *device);

    public:
        std::vector<ElecNekoMesh *> m_meshes;
        std::unordered_map<std::string, int> m_meshIndexByKey;

        std::vector<Material> m_materials;
        std::vector<Texture *> m_textures;
        Texture *defaultAlbedo;
        Texture *defaultNormal;
        Texture *defaultMetalRough;
        Texture *defaultEmission;

        std::vector<ElecNekoMeshInstance> m_meshInstances;

        std::vector<Light *> m_lights;

        std::unordered_map<std::string, int> m_textureCache;

        Camera *m_cam;
    };
} // namespace ElecNeko
