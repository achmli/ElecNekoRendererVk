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

    struct RenderOption
    {
        RenderOption()
        {
            renderResolution = iVec2(1280, 720);
            windowResolution = iVec2(1280, 720);
            uniformLightCol = Vec3(.3f, .3f, .3f);
            backgroundLightCol = Vec3(1.f, 1.f, 1.f);
            texArrayWidth = 2048;
            texArrayHeight = 2048;
            enableTonemap = true;
            enableAces = false;
            simpleAcesFit = false;
            enableEnvMap = false;
            enableUniformLight = false;
            hideEmitters = false;
            enableBackground = false;
            transparentBackground = false;
            independentRenderSize = false;
            enableRoughnessMollification = false;
            enableVolumeMIS = false;
            envMapIntensity = 1.f;
            envMapRot = 0.f;
            roughnessMollificationAmt = 0.f;
        }

        iVec2 renderResolution;
        iVec2 windowResolution;
        Vec3 uniformLightCol;
        Vec3 backgroundLightCol;
        int texArrayWidth;
        int texArrayHeight;
        bool enableTonemap;
        bool enableAces;
        bool simpleAcesFit;
        bool enableEnvMap;
        bool enableUniformLight;
        bool hideEmitters;
        bool enableBackground;
        bool transparentBackground;
        bool independentRenderSize;
        bool enableRoughnessMollification;
        bool enableVolumeMIS;
        float envMapIntensity;
        float envMapRot;
        float roughnessMollificationAmt;
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
        // options
        RenderOption renderOption;

        // Meshes
        std::vector<std::unique_ptr<Mesh>> meshes;

        // Scene Mesh Data
        std::vector<uint32_t> opaqueIndices;
        std::vector<vert_t> opaqueVertices;

        std::vector<uint32_t> maskIndices;
        std::vector<vert_t> maskVertices;

        std::vector<uint32_t> blendIndices;
        std::vector<vert_t> blendVertices;

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
