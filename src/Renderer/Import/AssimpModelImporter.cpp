// src/Renderer/Import/AssimpModelImporter.cpp
#include "Renderer/Import/AssimpModelImporter.h"

#include "Renderer/Mesh/AssimpStaticMeshLoader.h"

#include <assimp/Importer.hpp>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

namespace ElecNeko
{
    namespace
    {
        static unsigned int BuildAssimpFlags(const ModelImportOptions &options)
        {
            unsigned int flags = 0;

            if (options.triangulate)
            {
                flags |= aiProcess_Triangulate;
            }

            if (options.generateNormals)
            {
                flags |= aiProcess_GenSmoothNormals;
            }

            if (options.generateTangents)
            {
                flags |= aiProcess_CalcTangentSpace;
            }

            if (options.flipUVs)
            {
                flags |= aiProcess_FlipUVs;
            }

            if (options.improveCacheLocality)
            {
                flags |= aiProcess_ImproveCacheLocality;
            }

            if (options.removeRedundantMaterials)
            {
                flags |= aiProcess_RemoveRedundantMaterials;
            }

            if (options.findInvalidData)
            {
                flags |= aiProcess_FindInvalidData;
            }

            return flags;
        }

        static ImportedModelMatrix ConvertAssimpMatrix(const aiMatrix4x4 &m)
        {
            ImportedModelMatrix out{};

            out.m[0] = m.a1;
            out.m[1] = m.a2;
            out.m[2] = m.a3;
            out.m[3] = m.a4;

            out.m[4] = m.b1;
            out.m[5] = m.b2;
            out.m[6] = m.b3;
            out.m[7] = m.b4;

            out.m[8] = m.c1;
            out.m[9] = m.c2;
            out.m[10] = m.c3;
            out.m[11] = m.c4;

            out.m[12] = m.d1;
            out.m[13] = m.d2;
            out.m[14] = m.d3;
            out.m[15] = m.d4;

            return out;
        }

        static std::string MakeNodeInstanceName(const aiNode *node, uint32_t meshSlotInNode)
        {
            std::string nodeName = node != nullptr && node->mName.length > 0 ? node->mName.C_Str() : "node";

            return nodeName + "_mesh_" + std::to_string(meshSlotInNode);
        }

        static std::string GetMaterialName(const aiMaterial *material, uint32_t materialIndex)
        {
            aiString name;

            if (material != nullptr && material->Get(AI_MATKEY_NAME, name) == AI_SUCCESS)
            {
                if (name.length > 0)
                {
                    return name.C_Str();
                }
            }

            return "material_" + std::to_string(materialIndex);
        }

        static bool GetTexturePath(const aiMaterial *material, aiTextureType type, std::string &outPath)
        {
            if (material == nullptr)
            {
                return false;
            }

            aiString path;

            if (material->GetTexture(type, 0, &path) != AI_SUCCESS)
            {
                return false;
            }

            if (path.length == 0)
            {
                return false;
            }

            outPath = path.C_Str();
            return true;
        }

        static void TryReadDiffuseColor(const aiMaterial *material, SceneMaterialDesc &outDesc)
        {
            if (material == nullptr)
            {
                return;
            }

            aiColor4D color;

            if (aiGetMaterialColor(material, AI_MATKEY_COLOR_DIFFUSE, &color) == AI_SUCCESS)
            {
                outDesc.baseColor = Vec3(color.r, color.g, color.b);
                outDesc.opacity = color.a;
            }
        }

        static void TryReadEmissionColor(const aiMaterial *material, SceneMaterialDesc &outDesc)
        {
            if (material == nullptr)
            {
                return;
            }

            aiColor4D color;

            if (aiGetMaterialColor(material, AI_MATKEY_COLOR_EMISSIVE, &color) == AI_SUCCESS)
            {
                outDesc.emission = Vec3(color.r, color.g, color.b);
            }
        }

        static void TryReadOpacity(const aiMaterial *material, SceneMaterialDesc &outDesc)
        {
            if (material == nullptr)
            {
                return;
            }

            float opacity = 1.0f;

            if (aiGetMaterialFloat(material, AI_MATKEY_OPACITY, &opacity) == AI_SUCCESS)
            {
                outDesc.opacity = opacity;
            }
        }

        static void TryReadShininessAsRoughness(const aiMaterial *material, SceneMaterialDesc &outDesc)
        {
            if (material == nullptr)
            {
                return;
            }

            float shininess = 0.0f;

            if (aiGetMaterialFloat(material, AI_MATKEY_SHININESS, &shininess) == AI_SUCCESS)
            {
                // Legacy materials often expose shininess instead of roughness.
                // This is only an approximate fallback.
                const float normalized = std::clamp(shininess / 100.0f, 0.0f, 1.0f);
                outDesc.roughness = std::clamp(1.0f - normalized, 0.045f, 1.0f);
            }
        }

        static SceneAlphaMode GuessAlphaMode(const SceneMaterialDesc &desc)
        {
            if (desc.opacity < 0.999f)
            {
                return SceneAlphaMode::Blend;
            }

            // if (!desc.baseColorTexture.empty())
            // {
            //     // Conservative default for alpha-textured imported models.
            //     // Real alpha mode should be read from glTF when we add PBR-specific parsing.
            //     return SceneAlphaMode::Mask;
            // }

            return SceneAlphaMode::Opaque;
        }

        static ImportedModelMaterial ImportMaterial(const aiMaterial *material, uint32_t materialIndex)
        {
            ImportedModelMaterial imported{};
            imported.localMaterialIndex = materialIndex;

            SceneMaterialDesc desc{};
            desc.name = GetMaterialName(material, materialIndex);

            TryReadDiffuseColor(material, desc);
            TryReadEmissionColor(material, desc);
            TryReadOpacity(material, desc);
            TryReadShininessAsRoughness(material, desc);

            std::string texturePath;

            if (GetTexturePath(material, aiTextureType_DIFFUSE, texturePath))
            {
                desc.baseColorTexture = texturePath;
            }

            if (GetTexturePath(material, aiTextureType_NORMALS, texturePath))
            {
                desc.normalTexture = texturePath;
            }
            else if (GetTexturePath(material, aiTextureType_HEIGHT, texturePath))
            {
                // Some OBJ/MTL files store normal maps as height/bump maps.
                desc.normalTexture = texturePath;
            }

            if (GetTexturePath(material, aiTextureType_EMISSIVE, texturePath))
            {
                desc.emissionTexture = texturePath;
            }

            // Metallic-roughness texture is intentionally not handled here yet.
            // Different formats expose it differently. We will add glTF-specific parsing later.

            desc.alphaMode = GuessAlphaMode(desc);

            imported.desc = std::move(desc);
            return imported;
        }

        static void ImportMaterials(const aiScene *scene, ImportedModel &outModel)
        {
            if (scene == nullptr)
            {
                return;
            }

            outModel.materials.reserve(scene->mNumMaterials);

            for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex)
            {
                const aiMaterial *material = scene->mMaterials[materialIndex];

                ImportedModelMaterial importedMaterial = ImportMaterial(material, materialIndex);

                outModel.materials.push_back(std::move(importedMaterial));
            }

            if (outModel.materials.empty())
            {
                ImportedModelMaterial fallback{};
                fallback.localMaterialIndex = 0;
                fallback.desc.name = "__imported_default_material";
                fallback.desc.baseColor = Vec3(1.0f, 1.0f, 1.0f);
                fallback.desc.roughness = 0.5f;
                fallback.desc.alphaMode = SceneAlphaMode::Opaque;
                outModel.materials.push_back(fallback);
            }
        }

        static void TraverseNodeInstances(const aiScene *scene, const aiNode *node, const aiMatrix4x4 &parentTransform, ImportedModel &outModel)
        {
            if (scene == nullptr || node == nullptr)
            {
                return;
            }

            const aiMatrix4x4 localToModel = parentTransform * node->mTransformation;

            for (uint32_t meshSlot = 0; meshSlot < node->mNumMeshes; ++meshSlot)
            {
                const uint32_t aiMeshIndex = node->mMeshes[meshSlot];

                if (aiMeshIndex >= outModel.meshes.size())
                {
                    continue;
                }

                ImportedModelInstance instance{};
                instance.name = MakeNodeInstanceName(node, meshSlot);
                instance.meshIndex = aiMeshIndex;
                instance.localToModel = ConvertAssimpMatrix(localToModel);

                outModel.instances.push_back(instance);
            }

            for (uint32_t childIndex = 0; childIndex < node->mNumChildren; ++childIndex)
            {
                TraverseNodeInstances(scene, node->mChildren[childIndex], localToModel, outModel);
            }
        }
    } // namespace

    ModelImportResult AssimpModelImporter::ImportModelFile(const std::filesystem::path &filePath, const ModelImportOptions &options)
    {
        ModelImportResult result{};

        Assimp::Importer importer;

        const unsigned int flags = BuildAssimpFlags(options);

        const aiScene *scene = importer.ReadFile(filePath.string(), flags);

        if (scene == nullptr)
        {
            result.success = false;
            result.errorMessage = importer.GetErrorString();
            return result;
        }

        if (scene->mNumMeshes == 0)
        {
            result.success = false;
            result.errorMessage = "Model contains no meshes.";
            return result;
        }

        result.model.sourceFile = filePath;
        result.model.baseDirectory = filePath.parent_path();

        ImportMaterials(scene, result.model);

        result.model.meshes.reserve(scene->mNumMeshes);

        for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex)
        {
            const aiMesh *aiMesh = scene->mMeshes[meshIndex];

            if (aiMesh == nullptr)
            {
                continue;
            }

            const std::string meshName = aiMesh->mName.length > 0 ? aiMesh->mName.C_Str() : ("mesh_" + std::to_string(meshIndex));

            // This is an imported-model-local material slot.
            // It must be remapped to MaterialHandle later by SceneAssetBuilder.
            const uint32_t materialSlot = aiMesh->mMaterialIndex;

            StaticMeshAsset meshAsset = AssimpStaticMeshLoader::BuildStaticMeshAssetFromAiMesh(aiMesh, meshName, materialSlot);

            ImportedModelMesh importedMesh{};
            importedMesh.name = meshName;
            importedMesh.mesh = std::move(meshAsset);

            result.model.meshes.push_back(std::move(importedMesh));
        }

        if (scene->mRootNode != nullptr)
        {
            aiMatrix4x4 identity;
            TraverseNodeInstances(scene, scene->mRootNode, identity, result.model);
        }

        // Some OBJ files may have meshes but no useful node hierarchy.
        // In that case, create one fallback instance per mesh.
        if (result.model.instances.empty())
        {
            for (uint32_t meshIndex = 0; meshIndex < result.model.meshes.size(); ++meshIndex)
            {
                ImportedModelInstance instance{};
                instance.name = result.model.meshes[meshIndex].name;
                instance.meshIndex = meshIndex;
                instance.localToModel = ImportedModelMatrix{};

                result.model.instances.push_back(instance);
            }
        }

        result.success = !result.model.meshes.empty() && !result.model.instances.empty() && !result.model.materials.empty();

        if (!result.success)
        {
            result.errorMessage = "Model import produced no usable mesh instances or materials.";
        }

        return result;
    }
} // namespace ElecNeko
