// src/Renderer/Import/AssimpModelImporter.h
#pragma once

#include "Renderer/Mesh/StaticMesh.h"
#include "Renderer/Scene/SceneLoadDesc.h"

#include <filesystem>
#include <string>
#include <vector>

namespace ElecNeko
{
    struct ModelImportOptions
    {
        bool triangulate = true;
        bool generateNormals = false;
        bool generateTangents = false;
        bool flipUVs = false;

        bool improveCacheLocality = false;
        bool removeRedundantMaterials = false;
        bool findInvalidData = false;
    };

    struct ImportedModelMatrix
    {
        // Row-major 4x4 matrix.
        // Conversion to engine Mat4 should happen in RenderSceneBuilder later.
        float m[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct ImportedModelMesh
    {
        std::string name;

        // CPU-side mesh asset.
        //
        // Important:
        // StaticMeshSection::materialIndex is currently used as an imported-model-local material slot.
        // It is NOT a global MaterialHandle and NOT a RenderScene material index.
        StaticMeshAsset mesh;
    };

    struct ImportedModelMaterial
    {
        // Imported-model-local material slot.
        // This index should match aiMaterial index.
        uint32_t localMaterialIndex = 0;

        // Reuse SceneMaterialDesc as an intermediate CPU material description.
        // Texture strings are relative paths from the model file directory when possible.
        SceneMaterialDesc desc;
    };

    struct ImportedModelInstance
    {
        std::string name;

        // Index into ImportedModel::meshes.
        uint32_t meshIndex = 0;

        // Full transform from model root to this node.
        ImportedModelMatrix localToModel;
    };

    struct ImportedModel
    {
        std::filesystem::path sourceFile;
        std::filesystem::path baseDirectory;

        std::vector<ImportedModelMesh> meshes;
        std::vector<ImportedModelMaterial> materials;
        std::vector<ImportedModelInstance> instances;
    };

    struct ModelImportResult
    {
        bool success = false;
        std::string errorMessage;

        ImportedModel model;
    };

    class AssimpModelImporter
    {
    public:
        static ModelImportResult ImportModelFile(const std::filesystem::path &filePath, const ModelImportOptions &options = ModelImportOptions{});
    };
} // namespace ElecNeko
