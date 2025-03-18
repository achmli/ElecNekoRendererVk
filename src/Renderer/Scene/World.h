//
// Created by ElecNekoSurface on 25-3-17.
//

#pragma once

#include <memory>
#include <string>
#include <vector>

#include "Camera.h"
#include "EnvironmentMap.h"
#include "Material.h"
#include "Mesh.h"
#include "Texture.h"

namespace ElecNeko
{
    enum LightType
    {
        RectLight,
        SphereLight,
        DistantLight
    };

    struct Light
    {
        Vec3 position;
        Vec3 emission;
        Vec3 u;
        Vec3 v;
        float radius;
        float area;
        float type;
    };

    class Scene
    {
    public:
        Scene() : envMap(nullptr), camera(nullptr), initialized(false), dirty(true) {}
        ~Scene() = default;

        int AddMesh(const std::string &filename);
        int AddTexture(const std::string &filename);
        int AddMaterial(const Material &material);
        int AddMeshInstance(const MeshInstance &meshInstance);
        int AddLight(const Light &light);

        void AddCamera(const Vec3 &eye, const Vec3 &lookAt, float fov);
        void AddEnvMap(const std::string &filename);

        void ProcessScene();
        void RebuildInstance();

    public:
        // Meshes
        std::vector<std::unique_ptr<Mesh>> meshes;

        // Scene Mesh Data
        std::vector<uint32_t> vertIndices;
        std::vector<vert_t> vertices;
        std::vector<Mat4> transforms;

        // Materials
        std::vector<Material> materials;

        // Instances
        std::vector<MeshInstance> meshInstances;

        // Lights
        std::vector<Light> lights;

        // Environment Map
        std::unique_ptr<EnvironmentMap> envMap;

        // Camera
        std::unique_ptr<Camera> camera;

        // Texture Data
        std::vector<std::unique_ptr<Texture>> textures;
        std::vector<unsigned char> textureMapsArray;

        bool initialized;
        bool dirty;
        // to check if scene elements need to be resent to GPU.
        bool instancesModified = false;
        bool envMapModified = false;
    };
} // namespace ElecNeko
