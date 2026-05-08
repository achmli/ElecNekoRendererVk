//
// Created by ElecNekoSurface on 25-11-13.
//

#include "ElecNekoScene.h"
#include "Loader/Material.h"

#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include "assimp/pbrmaterial.h"

namespace ElecNeko
{
    static Vec3 AiColorToVec3(const aiColor4D &c) { return Vec3(static_cast<float>(c.r), static_cast<float>(c.g), static_cast<float>(c.b)); }

    static bool AiGetMaterialFloat(aiMaterial *m, const char *key, unsigned int type, unsigned int idx, float &out)
    {
        aiReturn r = m->Get(key, type, idx, out);
        return r == AI_SUCCESS;
    }

    static std::string GetAiTexturePath(const aiString &s) { return std::string(s.C_Str()); }

    // safe lowercase helper
    static std::string ToLowerStr(const std::string &s)
    {
        std::string out = s;
        for (char &c: out)
        {
            c = static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
        }
        return out;
    }

    static std::string Trim(std::string_view v)
    {
        size_t b = 0, e = v.size();
        while (b < e && std::isspace((unsigned char) v[b]))
        {
            ++b;
        }
        while (e > b && std::isspace((unsigned char) v[e - 1]))
        {
            --e;
        }
        return std::string(v.substr(b, e - b));
    }

    static std::vector<std::string> ReadBlock(std::ifstream &ifs, const std::string &firstLine)
    {
        std::vector<std::string> out;

        // If the opening line contains '{' then we start reading subsequent lines until matching '}'
        // If it does not, search for the next line with '{' then read.
        bool inBlock = (firstLine.find('{') != std::string::npos);
        if (!inBlock)
        {
            std::string line;
            while (std::getline(ifs, line))
            {
                auto t = Trim(line);
                if (t == "{" || (!t.empty() && t.front() == '{'))
                {
                    inBlock = true;
                    break;
                }
                if (t.empty())
                    continue;
            }
            if (!inBlock)
                return out;
        }

        std::string line;
        while (std::getline(ifs, line))
        {
            std::string t = Trim(line);
            if (t.empty())
                continue;
            if (t.find('}') != std::string::npos)
                break;
            out.push_back(t);
        }

        return out;
    }

    // parse a vec3 from a single line token like: "position -0.1 0.2 1.0"
    static std::optional<std::array<float, 3>> ParseVec3(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
            return std::nullopt;

        std::array<float, 3> v{0, 0, 0};
        if (iss >> v[0] >> v[1] >> v[2])
            return v;

        return std::nullopt;
    }

    // parse two ints
    static std::optional<std::pair<int, int>> ParseTwoInts(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return std::nullopt;
        }
        int a, b;
        if (iss >> a >> b)
        {
            return std::pair<int, int>{a, b};
        }
        return std::nullopt;
    }

    static std::string ParseSingleToken(const std::string &line)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return "";
        }
        /*std::string token;
        iss >> token;
        return token;*/
        std::string rest;
        std::getline(iss, rest);
        // trim rest
        size_t b = 0, e = rest.size();
        while (b < e && std::isspace((unsigned char) rest[b]))
        {
            ++b;
        }
        while (e > b && std::isspace((unsigned char) rest[e - 1]))
        {
            --e;
        }
        return std::string(rest.substr(b, e - b));
    }

    static float ParseSingleFloat(const std::string &line, float fallback = 0.0f)
    {
        std::istringstream iss(line);
        std::string key;
        if (!(iss >> key))
        {
            return fallback;
        }
        float v;
        if (iss >> v)
        {
            return v;
        }
        return fallback;
    }

    static std::string NormalizePathForKey(const std::string &p)
    {
        try
        {
            return std::filesystem::weakly_canonical(std::filesystem::path(p)).string();
        } catch (...)
        {
            return p;
        }
    }

    // helper: convert aiMatrix 4x4 ->Mat4
    static Mat4 AiToMat4(const aiMatrix4x4 &m)
    {
        Mat4 out;
        out.rows[0] = Vec4(static_cast<float>(m.a1), static_cast<float>(m.a2), static_cast<float>(m.a3), static_cast<float>(m.a4));
        out.rows[1] = Vec4(static_cast<float>(m.b1), static_cast<float>(m.b2), static_cast<float>(m.b3), static_cast<float>(m.b4));
        out.rows[2] = Vec4(static_cast<float>(m.c1), static_cast<float>(m.c2), static_cast<float>(m.c3), static_cast<float>(m.c4));
        out.rows[3] = Vec4(static_cast<float>(m.d1), static_cast<float>(m.d2), static_cast<float>(m.d3), static_cast<float>(m.d4));
        return out;
    }

    // Mat4 -> aiMatrix4x4
    inline aiMatrix4x4 Mat4ToAiMatrix4x4(const Mat4 &m) noexcept
    {
        aiMatrix4x4 out;
        out.a1 = static_cast<float>(m.rows[0].x);
        out.a2 = static_cast<float>(m.rows[0].y);
        out.a3 = static_cast<float>(m.rows[0].z);
        out.a4 = static_cast<float>(m.rows[0].w);

        out.b1 = static_cast<float>(m.rows[1].x);
        out.b2 = static_cast<float>(m.rows[1].y);
        out.b3 = static_cast<float>(m.rows[1].z);
        out.b4 = static_cast<float>(m.rows[1].w);

        out.c1 = static_cast<float>(m.rows[2].x);
        out.c2 = static_cast<float>(m.rows[2].y);
        out.c3 = static_cast<float>(m.rows[2].z);
        out.c4 = static_cast<float>(m.rows[2].w);

        out.d1 = static_cast<float>(m.rows[3].x);
        out.d2 = static_cast<float>(m.rows[3].y);
        out.d3 = static_cast<float>(m.rows[3].z);
        out.d4 = static_cast<float>(m.rows[3].w);

        return out;
    }

    static void TraverseAssimpNodesAndCreateInstances(const aiScene *scene, const aiNode *node, const aiMatrix4x4 &parentTransform, ElecNekoScene *world,
                                                      const std::vector<int> &aiToWorldMesh, const std::vector<int> &aiToWorldMat)
    {
        if (!node)
        {
            return;
        }

        aiMatrix4x4 global = parentTransform * node->mTransformation;

        for (uint32_t i = 0; i < node->mNumMeshes; i++)
        {
            uint32_t aiMeshIndex = node->mMeshes[i];
            if (aiMeshIndex >= aiToWorldMesh.size())
            {
                continue;
            }
            int worldMeshIndex = aiToWorldMesh[aiMeshIndex];
            if (worldMeshIndex < 0 || worldMeshIndex >= world->m_meshes.size())
            {
                continue;
            }

            int worldMatIndex = -1;
            const aiMesh *am = scene->mMeshes[aiMeshIndex];
            if (am && am->mMaterialIndex >= 0 && am->mMaterialIndex < aiToWorldMat.size())
            {
                worldMatIndex = aiToWorldMat[am->mMaterialIndex];
            }

            std::string instanceName = world->m_meshes[worldMeshIndex]->name + "_instance";
            Mat4 xform = AiToMat4(global);

            world->m_packedInstances.emplace_back(xform, worldMatIndex, worldMeshIndex, 0, 0);
        }

        for (uint32_t i = 0; i < node->mNumChildren; i++)
        {
            TraverseAssimpNodesAndCreateInstances(scene, node->mChildren[i], global, world, aiToWorldMesh, aiToWorldMat);
        }
    }

    static std::string SaveEmbeddedTextureToTemFile(const aiTexture *atex, const std::filesystem::path &basePath, int embIndex)
    {
        if (!atex)
        {
            return "";
        }

        if (atex->mHeight == 0 && atex->pcData)
        {
            std::string fmt = "";
            if (atex->achFormatHint && atex->achFormatHint[0] != '\0')
            {
                fmt = std::string(atex->achFormatHint);
                auto pos = fmt.find('\0');
                if (pos != std::string::npos)
                {
                    fmt.resize(pos);
                }
            }
            if (fmt.empty())
            {
                fmt = "png"; // default to png
            }

            std::string name = "__embedded_tex_" + std::to_string(embIndex) + "." + fmt;
            std::filesystem::path outPath = basePath / name;

            std::ofstream ofs(outPath, std::ios::binary);
            if (!ofs)
            {
                return "";
            }
            ofs.write(reinterpret_cast<const char *>(atex->pcData), atex->mWidth);
            ofs.close();
            return outPath.string();
        }

        return "";
    }

    static int ResolveAndLoadTextureFromMaterial(aiMaterial *aim, const aiScene *scene, ElecNekoScene *world, DeviceContext *device,
                                                 const std::filesystem::path &basePath, const std::vector<aiTextureType> &tryTypes)
    {
        aiString texPath;

        for (auto type: tryTypes)
        {
            if (aim->GetTexture(type, 0, &texPath) == AI_SUCCESS)
            {
                std::string p = texPath.C_Str();
                if (p.empty())
                {
                    continue;
                }

                if (p.front() == '*')
                {
                    int emb = 0;
                    try
                    {
                        emb = std::stoi(p.substr(1));
                    } catch (...)
                    {
                        continue;
                    }
                    if (emb >= 0 && emb < scene->mNumTextures)
                    {
                        const aiTexture *atex = scene->mTextures[emb];
                        std::string tempTexPath = SaveEmbeddedTextureToTemFile(atex, basePath, emb);
                        if (!tempTexPath.empty())
                        {
                            // todo: LoadTexture
                            return world->LoadTexture(device, tempTexPath);
                        }
                    }
                }
                else
                {
                    std::filesystem::path ppath(p);
                    std::filesystem::path resolved;
                    if (ppath.is_absolute())
                    {
                        resolved = ppath;
                    }
                    else
                    {
                        resolved = (basePath / ppath).lexically_normal();
                    }
                    // todo: LoadTexture
                    return world->LoadTexture(device, resolved.string());
                }
            }
        }
        return -1;
    }


    // only for gltf maybe... can load obj, but not so suitable
    static Material ConvertAssimpMaterial(aiMaterial *aim, const aiScene *scene, ElecNekoScene *world, DeviceContext *device,
                                          const std::filesystem::path &basePath)
    {
        Material mat;

        aiString name;
        if (aim->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
        {
            mat.name = std::string(name.C_Str());
        }

        aiColor4D diffuse(1.f, 1.f, 1.f, 1.f);
        if (aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_BASE_COLOR_FACTOR, diffuse) == AI_SUCCESS)
        {
            mat.baseColor = Vec3((float) diffuse.r, (float) diffuse.g, (float) diffuse.b);
            mat.opacity = (float) diffuse.a;
        }

        aiColor3D emissive(0, 0, 0);
        if (aim->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS)
        {
            mat.emission = Vec3((float) emissive.r, (float) emissive.g, (float) emissive.b);
        }


        float metal = 0.f, rough = .5f;
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_METALLIC_FACTOR, metal);
        aim->Get(AI_MATKEY_GLTF_PBRMETALLICROUGHNESS_ROUGHNESS_FACTOR, rough);
        mat.metallic = metal;
        mat.roughness = rough;

        float alphaCutoff = 0.5f;
        if (aim->Get(AI_MATKEY_GLTF_ALPHACUTOFF, alphaCutoff) == AI_SUCCESS)
        {
            mat.alphaCutoff = alphaCutoff;
        }
        uint32_t alphaMode = 0;
        if (aim->Get(AI_MATKEY_GLTF_ALPHAMODE, alphaMode) == AI_SUCCESS)
        {
            if (alphaMode == 1)
            {
                mat.alphaMode = AlphaMode::Mask;
            }
            else if (alphaMode == 2)
            {
                mat.alphaMode = AlphaMode::Blend;
            }
            else
            {
                mat.alphaMode = AlphaMode::Opaque;
            }
        }

        float specTrans = 0.f;
        if (aim->Get(AI_MATKEY_GLTF_MATERIAL_TRANSMISSION_FACTOR, specTrans) == AI_SUCCESS)
        {
            mat.specTrans = specTrans;
        }


        {
            std::vector<aiTextureType> tryTypes = {aiTextureType_BASE_COLOR, aiTextureType_DIFFUSE};
            int texId = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, tryTypes);
            if (texId > 0)
            {
                mat.baseColorTexId = texId;
            }
        }
        {
            std::vector<aiTextureType> tryTypes = {aiTextureType_NORMALS, aiTextureType_HEIGHT};
            int texId = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, tryTypes);
            if (texId > 0)
            {
                mat.normalMapTexId = texId;
            }
        }
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_GLTF_METALLIC_ROUGHNESS, aiTextureType_METALNESS, aiTextureType_DIFFUSE_ROUGHNESS};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
            if (tid >= 0)
                mat.metallicRoughtnessTexId = tid;
        }
        {
            std::vector<aiTextureType> trySlots = {aiTextureType_EMISSIVE};
            int tid = ResolveAndLoadTextureFromMaterial(aim, scene, world, device, basePath, trySlots);
            if (tid >= 0)
                mat.emissionmapTexId = tid;
        }

        if (mat.alphaMode == AlphaMode::Opaque)
        {
            if (mat.opacity < 0.999f)
            {
                mat.alphaMode = AlphaMode::Blend;
            }
            else if (mat.alphaCutoff > 0.001f)
            {
                mat.alphaMode = AlphaMode::Mask;
            }
        }

        return mat;
    }


} // namespace ElecNeko
