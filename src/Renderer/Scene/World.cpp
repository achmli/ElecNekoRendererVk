//
// Created by ElecNekoSurface on 25-3-17.
//

#include "World.h"
#include <iostream>
#include <vector>
#include "stb_image.h"
#include "stb_image_resize.h"

namespace ElecNeko
{
    int Scene::AddMesh(const std::string &filename)
    {
        // check if the mesh already has been loaded
        for (int i = 0; i < meshes.size(); i++)
        {
            if (meshes[i]->name == filename)
            {
                return i;
            }
        }

        int id = static_cast<int>(meshes.size());

        const auto mesh = std::make_unique<Mesh>();

        printf("Loading Model: %s\n", filename.c_str());
        if (mesh->LoadFromFile(filename))
        {
            meshes.push_back(std::move(mesh));
        }
        else
        {
            printf("Failed to load model %s\n", filename.c_str());
            id = -1;
        }

        return id;
    }

    int Scene::AddTexture(const std::string &filename)
    {
        // check if texture has been loaded
        for (int i = 0; i < textures.size(); i++)
        {
            if (textures[i]->name == filename)
            {
                return i;
            }
        }

        int id = static_cast<int>(textures.size());
        auto texture = std::make_unique<Texture>();

        printf("Loading texture %s\n", filename.c_str());
        if (texture->LoadTexture(filename))
        {
            textures.push_back(std::move(texture));
        }
        else
        {
            printf("Failed to load texture %s\n", filename.c_str());
            id = -1;
        }

        return id;
    }

    int Scene::AddMaterial(const Material &material)
    {
        const int id = static_cast<int>(materials.size());
        materials.emplace_back(material);
        return id;
    }

    void Scene::AddCamera(const Vec3 &eye, const Vec3 &lookAt, float fov)
    {
        camera = std::make_unique<Camera>(eye, lookAt, fov);
    }

    void Scene::AddEnvMap(const std::string &filename)
    {
        envMap = std::make_unique<EnvironmentMap>();
        if (envMap->LoadMap(filename))
        {
            printf("HDR %s Loaded\n", filename.c_str());
        }
        else
        {
            printf("Failed to load HDR\n");
            envMap.reset();
        }
        envMapModified = true;
        dirty = true;
    }

    int Scene::AddMeshInstance(const MeshInstance &meshInstance)
    {
        const int id = static_cast<int>(meshInstances.size());
        meshInstances.emplace_back(meshInstance);
        return id;
    }

    int Scene::AddLight(const Light &light)
    {
        const int id = static_cast<int>(lights.size());
        lights.emplace_back(light);
        return id;
    }

    void Scene::RebuildInstance()
    {
        // Copy transforms
        for (int i = 0; i < meshInstances.size(); i++)
        {
            transforms[i] = meshInstances[i].transform;
        }

        instancesModified = true;
        dirty = false;
    }

    void Scene::ProcessScene()
    {
        // Copy Mesh Data
        int verticesCnt = 0;
        for (const auto &mesh: meshes)
        {
            int numIndices = mesh->vertices.size();
        }
    }

} // namespace ElecNeko
