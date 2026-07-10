// src/Renderer/Scene/SceneLoadDesc.h
#pragma once

#include "Math/Vector.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ElecNeko
{
    enum class SceneAssetFormat
    {
        Unknown,
        Obj,
        Gltf,
        Fbx,
        Other
    };

    enum class SceneAlphaMode
    {
        Opaque,
        Mask,
        Blend
    };

    enum class SceneMediumType
    {
        None,
        Absorb,
        Scatter
    };

    struct SceneQuatDesc
    {
        float x = 0.0f;
        float y = 0.0f;
        float z = 0.0f;
        float w = 1.0f;
    };

    struct SceneTransformDesc
    {
        Vec3 position = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 scale = Vec3(1.0f, 1.0f, 1.0f);

        // The legacy scene format stores rotation as a quaternion:
        // rotation x y z w
        SceneQuatDesc rotation;

        bool hasPosition = false;
        bool hasScale = false;
        bool hasRotation = false;
    };

    struct SceneRendererDesc
    {
        int resolutionX = 1280;
        int resolutionY = 720;

        int windowResolutionX = 1280;
        int windowResolutionY = 720;

        int maxDepth = 1;
        int tileWidth = 0;
        int tileHeight = 0;

        std::string envMapFile;
        float envMapIntensity = 1.0f;

        bool independentRenderSize = false;
    };

    struct SceneCameraDesc
    {
        Vec3 position = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 lookAt = Vec3(0.0f, 0.0f, -1.0f);

        float fov = 60.0f;

        bool valid = false;
    };

    struct SceneMaterialDesc
    {
        std::string name;

        Vec3 baseColor = Vec3(1.0f, 1.0f, 1.0f);
        float opacity = 1.0f;

        Vec3 emission = Vec3(0.0f, 0.0f, 0.0f);

        float metallic = 0.0f;
        float roughness = 0.5f;

        float specularTint = 0.0f;
        float specTrans = 0.0f;
        float anisotropic = 0.0f;
        float subsurface = 0.0f;

        float sheen = 0.0f;
        float sheenTint = 0.0f;

        float clearcoat = 0.0f;
        float clearcoatGloss = 1.0f;

        float ior = 1.5f;

        SceneMediumType mediumType = SceneMediumType::None;
        float mediumDensity = 0.0f;
        Vec3 mediumColor = Vec3(1.0f, 1.0f, 1.0f);
        float mediumAnisotropy = 0.0f;

        SceneAlphaMode alphaMode = SceneAlphaMode::Opaque;
        float alphaCutoff = 0.5f;

        std::string baseColorTexture;
        std::string normalTexture;
        std::string metalRoughTexture;
        std::string emissionTexture;
    };

    struct SceneMeshInstanceDesc
    {
        std::string name;
        std::string file;
        std::string materialName;

        SceneTransformDesc transform;

        bool visible = true;
    };

    struct SceneModelInstanceDesc
    {
        std::string name;
        std::string file;

        SceneAssetFormat format = SceneAssetFormat::Unknown;
        SceneTransformDesc transform;

        bool visible = true;
    };

    enum class SceneLightType
    {
        Unknown,
        Quad,
        Sphere,
        Directional,
        Point
    };

    struct SceneLightDesc
    {
        std::string name;

        SceneLightType type = SceneLightType::Unknown;

        Vec3 position = Vec3(0.0f, 0.0f, 0.0f);
        Vec3 direction = Vec3(0.0f, -1.0f, 0.0f);
        Vec3 emission = Vec3(1.0f, 1.0f, 1.0f);

        float intensity = 1.0f;
        float radius = 1.0f;

        Vec3 u = Vec3(1.0f, 0.0f, 0.0f);
        Vec3 v = Vec3(0.0f, 1.0f, 0.0f);
    };

    struct SceneLoadDesc
    {
        std::filesystem::path sceneFile;
        std::filesystem::path baseDirectory;

        SceneRendererDesc renderer;
        SceneCameraDesc camera;

        std::vector<SceneMaterialDesc> materials;
        std::vector<SceneMeshInstanceDesc> meshInstances;
        std::vector<SceneModelInstanceDesc> modelInstances;
        std::vector<SceneLightDesc> lights;
    };
} // namespace ElecNeko
