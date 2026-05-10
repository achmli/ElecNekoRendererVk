#pragma once

#include "Loader/Mesh.h"

#include "RHI/RHICommandList.h"

#include <cassert>
#include <cstring>
#include <filesystem>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <vulkan/vulkan.h>

namespace ElecNeko
{
    struct SubMesh
    {
        uint32_t indexOffset = 0;
        uint32_t indexCount = 0;
        int materialId = 0;
    };

    struct ElecNekoModel
    {
        std::string name;
        std::vector<VVertex> m_vertices;
        std::vector<uint32_t> m_indices;
        std::vector<SubMesh> subMeshes;

        Buffer m_vertexBuffer;
        Buffer m_indexBuffer;

        RHI::BufferHandle vertexBufferHandle;
        RHI::BufferHandle indexBufferHandle;

        bool MakeVBO(DeviceContext *device);
        void Cleanup(DeviceContext *device);
    };

    struct InstanceCPU
    {
        Mat4 model;
        int meshId;
        int materialId;
    };

#pragma pack(push, 1)
    struct InstanceGPU
    {
        float modelRow0[4];
        float modelRow1[4];
        float modelRow2[4];
        float modelRow3[4];

        uint32_t materialId;
        uint32_t padding[3];
    };
#pragma pack(pop)
    static_assert(sizeof(InstanceGPU) % 16 == 0, "InstanceGPU size must be multiple of 16");

    struct MeshIndirectInfo
    {
        uint32_t indirectOffsetInBytes = 0;
        uint32_t drawCount = 0;
    };

    class ElecNekoWorld
    {
    public:
        ElecNekoWorld() = default;
        ~ElecNekoWorld() = default;

        std::vector<ElecNekoModel *> m_models;
        std::unordered_map<std::string, int> m_modelIndexByKey;

        std::vector<Material> m_materials;
        std::vector<Texture *> m_textures;
        Texture *defaultAlbedo = nullptr;
        Texture *defaultNormal = nullptr;
        Texture *defaultMetalRough = nullptr;
        Texture *defaultEmission = nullptr;

        std::vector<InstanceCPU> m_instances;
        Camera *m_cam = nullptr;

        Buffer instanceBuffer;
        Buffer indirectBuffer;

        RHI::BufferHandle instanceBufferHandle;
        RHI::BufferHandle indirectBufferHandle;

        uint32_t indirectCount = 0;
        std::unordered_map<int, MeshIndirectInfo> meshIndirectInfos;

        std::unordered_map<std::string, int> m_textureChace;

        // Material and texture system for rendering (keeps GPU-driven DrawIndirect)
        Buffer materialBuffer;
        TextureArray *textureArray = nullptr;

        int LoadModelGeometryOnly(DeviceContext *device, const std::string &filename, int overrideMaterialId = 0);
        int LoadModelWithMaterials(DeviceContext *device, const std::string &filename, Mat4 transMat);
        bool LoadSceneFromFile(DeviceContext *device, const std::string &filename);
        void CreateDefaultTextures(DeviceContext *device);
        int EnsureTextureCached(DeviceContext *device, const std::string &filename);
        int AddTexture(DeviceContext *device, const std::string &filename);
        int AddMaterial(const Material &material);
        void AddCamera(const Vec3 &pos, const Vec3 &lookAt, float fov, float aspectRatio = (9.f / 16.f), float zNear = .1f, float zFar = 1000.f);

        bool BuildInstanceAndIndirectBuffers(DeviceContext *device);

        bool BuildMaterialBuffers(DeviceContext *device);

        // void DrawIndirect(VkCommandBuffer cmdBuffer, std::function<void(int)> bindMaterialFunc = nullptr);
        void DrawIndirect(RHICommandList &cmd, std::function<void(int)> bindMaterialFunc = nullptr);

        void UnloadScene(DeviceContext *device);
        void Cleanup(DeviceContext *device);

    private:
        static Mat4 AiToMat4(const aiMatrix4x4 &aiMat);
        static aiMatrix4x4 Mat4ToAiMatrix4x4(const Mat4 &mat);
        static std::string NormalizePathForKey(const std::string &path);
        static std::string SaveEmbeddedTextureToTempFile(const aiTexture *texture, const std::filesystem::path &modelFilePath, int embIndex);
        static int ResolveAndLoadTextureFromMaterial(DeviceContext *device, aiMaterial *aim, const aiScene *scene, const std::filesystem::path &basepath,
                                                     const std::vector<aiTextureType> &tryTypes, ElecNekoWorld *world);
        static Material ConvertAssimpMaterial(DeviceContext *device, aiMaterial *aim, const aiScene *scene, const std::filesystem::path &basepath,
                                              ElecNekoWorld *world);
        static void TraverseAssimpNodesAndCreateInstance(const aiScene *scene, const aiNode *node, const aiMatrix4x4 &parentTransform,
                                                         const std::vector<int> &aiToWorldMesh, const std::vector<int> &aiToWorldMat, ElecNekoWorld *world);

        static std::string Trim(std::string_view v);
        static std::vector<std::string> ReadBlock(std::ifstream &ifs, const std::string &firstLine);
        static std::optional<std::array<float, 3>> ParseVec3(const std::string &line);
        static std::optional<std::pair<int, int>> ParseTwoInts(const std::string &line);
        static std::string ParseSingleToken(const std::string &line);
        static float ParseSingleFloat(const std::string &line, float fallback = 0.0f);
    };
} // namespace ElecNeko
