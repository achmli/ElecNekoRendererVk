// src/Renderer/Scene/SceneAssetBuilder.cpp
#include "Renderer/Scene/SceneAssetBuilder.h"

#include "Renderer/Assets/AssetManager.h"
#include "Renderer/Import/AssimpGeometryImporter.h"
#include "Renderer/Import/AssimpModelImporter.h"

#include <filesystem>
#include <iostream>
#include <unordered_map>

namespace ElecNeko
{
    namespace
    {
        static std::string MakeCanonicalPathKey(const std::filesystem::path &path)
        {
            std::error_code ec;
            std::filesystem::path canonicalPath = std::filesystem::weakly_canonical(path, ec);

            if (ec)
            {
                canonicalPath = path.lexically_normal();
            }

            return canonicalPath.generic_string();
        }

        static std::string MakeGeometryMeshKey(const std::filesystem::path &meshPath, const SceneAssetBuilderOptions &options)
        {
            std::string key = MakeCanonicalPathKey(meshPath);

            key += "|geometry";
            key += options.triangulate ? "|tri" : "|no_tri";
            key += options.generateNormals ? "|normals" : "|no_normals";
            key += options.generateTangents ? "|tangents" : "|no_tangents";
            key += options.flipUVs ? "|flip_uv" : "|no_flip_uv";
            key += options.mergeGeometryMeshes ? "|merge" : "|no_merge";
            key += options.deduplicateVertices ? "|dedup" : "|no_dedup";

            return key;
        }

        static GeometryImportOptions MakeGeometryImportOptions(const SceneAssetBuilderOptions &options)
        {
            GeometryImportOptions importOptions{};
            importOptions.triangulate = options.triangulate;
            importOptions.generateNormals = options.generateNormals;
            importOptions.generateTangents = options.generateTangents;
            importOptions.flipUVs = options.flipUVs;
            importOptions.mergeMeshes = options.mergeGeometryMeshes;
            importOptions.deduplicateVertices = options.deduplicateVertices;
            return importOptions;
        }

        static MaterialHandle FindMaterialHandleByName(const std::unordered_map<std::string, MaterialHandle> &materialByName, const std::string &name)
        {
            auto it = materialByName.find(name);

            if (it != materialByName.end())
            {
                return it->second;
            }

            return MaterialHandle{};
        }

        static MaterialHandle CreateFallbackMaterial(AssetManager &assetManager, const std::string &name)
        {
            MaterialAssetDesc desc{};
            desc.name = name.empty() ? "__default_scene_material" : name;
            desc.baseColor = Vec3(1.0f, 1.0f, 1.0f);
            desc.roughness = 0.5f;
            desc.metallic = 0.0f;
            desc.alphaMode = MaterialAlphaMode::Opaque;
            return assetManager.CreateMaterial(desc);
        }

        static std::string MakeModelMeshKey(const std::filesystem::path &modelPath, uint32_t meshIndex, const SceneAssetBuilderOptions &options)
        {
            std::string key = MakeCanonicalPathKey(modelPath);

            key += "|model_mesh_";
            key += std::to_string(meshIndex);
            key += options.triangulate ? "|tri" : "|no_tri";
            key += options.generateNormals ? "|normals" : "|no_normals";
            key += options.generateTangents ? "|tangents" : "|no_tangents";
            key += options.flipUVs ? "|flip_uv" : "|no_flip_uv";

            return key;
        }

        static SceneAssetMatrix ConvertImportedModelMatrixToSceneAssetMatrix(const ImportedModelMatrix &src)
        {
            SceneAssetMatrix dst{};

            for (int i = 0; i < 16; ++i)
            {
                dst.m[i] = src.m[i];
            }

            return dst;
        }

        static ModelImportOptions MakeModelImportOptions(const SceneAssetBuilderOptions &options)
        {
            ModelImportOptions importOptions{};
            importOptions.triangulate = options.triangulate;
            importOptions.generateNormals = options.generateNormals;
            importOptions.generateTangents = options.generateTangents;
            importOptions.flipUVs = options.flipUVs;

            importOptions.improveCacheLocality = false;
            importOptions.removeRedundantMaterials = false;
            importOptions.findInvalidData = false;

            return importOptions;
        }

        static void RemapImportedMaterialTexturePaths(SceneMaterialDesc &material, const std::filesystem::path &modelBaseDirectory)
        {
            if (!material.baseColorTexture.empty())
            {
                material.baseColorTexture = (modelBaseDirectory / material.baseColorTexture).lexically_normal().string();
            }

            if (!material.normalTexture.empty())
            {
                material.normalTexture = (modelBaseDirectory / material.normalTexture).lexically_normal().string();
            }

            if (!material.metalRoughTexture.empty())
            {
                material.metalRoughTexture = (modelBaseDirectory / material.metalRoughTexture).lexically_normal().string();
            }

            if (!material.emissionTexture.empty())
            {
                material.emissionTexture = (modelBaseDirectory / material.emissionTexture).lexically_normal().string();
            }
        }

        static bool AppendModelInstanceAssets(const SceneModelInstanceDesc &modelDesc, const SceneLoadDesc &sceneDesc, AssetManager &assetManager,
                                              const SceneAssetBuilderOptions &options, SceneAssetBuildResult &outBuildData, std::string &outErrorMessage)
        {
            if (modelDesc.file.empty())
            {
                std::cerr << "[SceneAssetBuilder] Model instance has empty file: " << modelDesc.name << "\n";
                return true;
            }

            const std::filesystem::path modelPath = sceneDesc.baseDirectory / modelDesc.file;

            ModelImportOptions modelImportOptions = MakeModelImportOptions(options);

            ModelImportResult importResult = AssimpModelImporter::ImportModelFile(modelPath, modelImportOptions);

            if (!importResult.success)
            {
                outErrorMessage = "Failed to import model file: " + modelPath.string() + " error: " + importResult.errorMessage;

                std::cerr << "[SceneAssetBuilder] " << outErrorMessage << "\n";
                return false;
            }

            ImportedModel &importedModel = importResult.model;

            std::vector<MaterialHandle> importedMaterialHandles;
            importedMaterialHandles.resize(importedModel.materials.size());

            for (const ImportedModelMaterial &importedMaterial: importedModel.materials)
            {
                SceneMaterialDesc materialDesc = importedMaterial.desc;

                // Texture paths from Assimp are relative to the model file directory.
                RemapImportedMaterialTexturePaths(materialDesc, importedModel.baseDirectory);

                MaterialHandle materialHandle = assetManager.CreateMaterialFromSceneDesc(materialDesc, std::filesystem::path{});

                if (importedMaterial.localMaterialIndex >= importedMaterialHandles.size())
                {
                    importedMaterialHandles.resize(importedMaterial.localMaterialIndex + 1);
                }

                importedMaterialHandles[importedMaterial.localMaterialIndex] = materialHandle;
            }

            if (importedMaterialHandles.empty())
            {
                MaterialHandle fallback = CreateFallbackMaterial(assetManager, "__imported_default_material");

                importedMaterialHandles.push_back(fallback);
            }

            std::vector<StaticMeshHandle> importedMeshHandles;
            importedMeshHandles.reserve(importedModel.meshes.size());

            for (uint32_t meshIndex = 0; meshIndex < importedModel.meshes.size(); ++meshIndex)
            {
                ImportedModelMesh &importedMesh = importedModel.meshes[meshIndex];

                const std::string meshKey = MakeModelMeshKey(modelPath, meshIndex, options);

                StaticMeshHandle meshHandle;

                if (!assetManager.TryGetStaticMeshHandle(meshKey, meshHandle))
                {
                    meshHandle = assetManager.AddStaticMeshAsset(meshKey, std::move(importedMesh.mesh));
                }

                importedMeshHandles.push_back(meshHandle);
            }

            MaterialSetHandle materialSetHandle = assetManager.CreateMaterialSet(importedMaterialHandles);

            for (const ImportedModelInstance &importedInstance: importedModel.instances)
            {
                if (importedInstance.meshIndex >= importedMeshHandles.size())
                {
                    continue;
                }

                SceneAssetMeshInstance instance{};
                instance.name = modelDesc.name.empty() ? importedInstance.name : (modelDesc.name + "_" + importedInstance.name);

                instance.mesh = importedMeshHandles[importedInstance.meshIndex];
                instance.materialSet = materialSetHandle;

                // Root transform from the .scene model block.
                instance.transform = modelDesc.transform;

                // Local transform from the imported model node hierarchy.
                instance.localToModel = ConvertImportedModelMatrixToSceneAssetMatrix(importedInstance.localToModel);
                instance.hasLocalToModel = true;

                instance.visible = modelDesc.visible;

                outBuildData.meshInstances.push_back(instance);
            }

            printf("[SceneAssetBuilder][Model] file=%s importedMeshes=%zu importedMaterials=%zu importedInstances=%zu totalSceneInstances=%zu\n",
                   modelPath.string().c_str(), importedModel.meshes.size(), importedModel.materials.size(), importedModel.instances.size(),
                   outBuildData.meshInstances.size());

            return true;
        }
    } // namespace

    // SceneAssetBuilderResult SceneAssetBuilder::BuildGeometryOnlySceneAssets(const SceneLoadDesc &sceneDesc, AssetManager &assetManager,
    //                                                                         const SceneAssetBuilderOptions &options)
    SceneAssetBuilderResult SceneAssetBuilder::BuildSceneAssets(const SceneLoadDesc &sceneDesc, AssetManager &assetManager,
                                                                const SceneAssetBuilderOptions &options)
    {
        SceneAssetBuilderResult result{};
        result.success = true;

        std::unordered_map<std::string, MaterialHandle> materialByName;
        materialByName.reserve(sceneDesc.materials.size());

        for (const SceneMaterialDesc &sceneMaterial: sceneDesc.materials)
        {
            MaterialHandle materialHandle = assetManager.CreateMaterialFromSceneDesc(sceneMaterial, sceneDesc.baseDirectory);

            if (!sceneMaterial.name.empty())
            {
                materialByName[sceneMaterial.name] = materialHandle;
            }
        }

        if (!options.importGeometryOnlyMeshes)
        {
            return result;
        }

        GeometryImportOptions importOptions = MakeGeometryImportOptions(options);

        for (const SceneMeshInstanceDesc &meshDesc: sceneDesc.meshInstances)
        {
            if (meshDesc.file.empty())
            {
                std::cerr << "[SceneAssetBuilder] Mesh instance has empty file: " << meshDesc.name << "\n";
                continue;
            }

            const std::filesystem::path meshPath = sceneDesc.baseDirectory / meshDesc.file;

            const std::string meshKey = MakeGeometryMeshKey(meshPath, options);

            StaticMeshHandle meshHandle;

            if (!assetManager.TryGetStaticMeshHandle(meshKey, meshHandle))
            {
                GeometryImportResult importResult = AssimpGeometryImporter::ImportGeometryFile(meshPath, importOptions);

                if (!importResult.success)
                {
                    result.success = false;

                    result.errorMessage = "Failed to import geometry file: " + meshPath.string() + " error: " + importResult.errorMessage;

                    std::cerr << "[SceneAssetBuilder] " << result.errorMessage << "\n";
                    continue;
                }

                meshHandle = assetManager.AddStaticMeshAsset(meshKey, std::move(importResult.mesh));
            }

            MaterialHandle materialHandle = FindMaterialHandleByName(materialByName, meshDesc.materialName);

            if (!materialHandle.IsValid())
            {
                std::cerr << "[SceneAssetBuilder] Missing material '" << meshDesc.materialName << "' for mesh '" << meshDesc.name
                          << "'. Creating fallback material.\n";

                materialHandle = CreateFallbackMaterial(assetManager, meshDesc.materialName);
            }

            std::vector<MaterialHandle> materialSlots;
            materialSlots.push_back(materialHandle);

            MaterialSetHandle materialSetHandle = assetManager.CreateMaterialSet(materialSlots);

            SceneAssetMeshInstance instance{};
            instance.name = meshDesc.name.empty() ? meshDesc.file : meshDesc.name;
            instance.mesh = meshHandle;
            instance.materialSet = materialSetHandle;
            instance.transform = meshDesc.transform;
            instance.visible = meshDesc.visible;

            result.buildData.meshInstances.push_back(instance);
        }

        if (options.importModelInstances)
        {
            for (const SceneModelInstanceDesc &modelDesc: sceneDesc.modelInstances)
            {
                std::string errorMessage;

                const bool ok = AppendModelInstanceAssets(modelDesc, sceneDesc, assetManager, options, result.buildData, errorMessage);

                if (!ok)
                {
                    result.success = false;

                    if (result.errorMessage.empty())
                    {
                        result.errorMessage = errorMessage;
                    }
                }
            }
        }

        return result;
    }
} // namespace ElecNeko
