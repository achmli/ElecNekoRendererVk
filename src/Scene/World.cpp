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
        {
            // Copy Mesh Data
            int opaqueVerticesCnt = 0;
            int blendVerticesCnt = 0;
            int maskVerticesCnt = 0;
            for (int i = 0; i < meshes.size(); i++)
            {
                const auto &mesh = meshes[i];
                int matID = meshInstances[i].materialId;
                const auto &alphaMode = materials[matID].alphaMode;

                // select target vector according to alpha mode
                std::vector<vert_t> *targetVertices = nullptr;
                std::vector<uint32_t> *targetIndices = nullptr;
                int *targetVertexCnt = nullptr;

                if (alphaMode == AlphaMode::Mask)
                {
                    targetVertices = &maskVertices;
                    targetIndices = &maskIndices;
                    targetVertexCnt = &maskVerticesCnt;
                }
                else if (alphaMode == AlphaMode::Blend)
                {
                    targetVertices = &blendVertices;
                    targetIndices = &blendIndices;
                    targetVertexCnt = &blendVerticesCnt;
                }
                else
                {
                    targetVertices = &opaqueVertices;
                    targetIndices = &opaqueIndices;
                    targetVertexCnt = &opaqueVerticesCnt;
                }

                if (targetIndices && targetVertices && targetVertexCnt)
                {
                    // process vertices
                    targetVertices->insert(targetVertices->end(), mesh->vertices.begin(), mesh->vertices.end());

                    // process offset of indices
                    const int numVertices = mesh->vertices.size();
                    const int numIndices = mesh->indices.size();

                    targetVertices->reserve(targetVertices->size() + numVertices);
                    targetIndices->reserve(targetIndices->size() + numIndices);

                    for (const uint32_t index: mesh->indices)
                    {
                        // calculate new index
                        uint32_t adjustedIndex = index + *targetVertexCnt;
                        targetIndices->emplace_back(adjustedIndex);
                    }

                    // update total vertices num
                    *targetVertexCnt += numVertices;
                }
            }
        }

        {
            // copy transform
            transforms.resize(meshInstances.size());
            for (int i = 0; i < meshInstances.size(); i++)
            {
                transforms[i] = meshInstances[i].transform;
            }
        }

        {
            // copy textures
            const int reqWidth = renderOption.texArrayWidth;
            const int reqHeight = renderOption.texArrayHeight;
            const int texBytes = reqWidth * reqHeight * 4;

            textureMapsArray.resize(texBytes * textures.size());

#pragma omp parallel for
            for (int i = 0; i < textures.size(); i++)
            {
                const int texWidth = textures[i]->width;
                const int texHeight = textures[i]->height;

                // resize textures to fit 2D texture array
                if (texWidth != reqWidth || texHeight != reqHeight)
                {
                    auto *resizedTex = new uint8_t[texBytes];
                    stbir_resize_uint8(&textures[i]->texData[0], texWidth, texHeight, 0, resizedTex, reqWidth,
                                       reqHeight, 0, 4);
                    std::copy_n(resizedTex, texBytes, &textureMapsArray[i * texBytes]);
                    delete[] resizedTex;
                }
                else
                {
                    std::copy_n(textures[i]->texData.begin(), textures.size(), &textureMapsArray[i * texBytes]);
                }
            }
        }

        {
            // add a default camera
            if (!camera)
            {
                AddCamera(Vec3(0, 0, 5), Vec3(0, 0, 0), 60.f);
            }
        }

        initialized = true;
    }
} // namespace ElecNeko
