// src/Renderer/Scene/SceneAssetBuilder.h
#pragma once

#include "Renderer/Scene/SceneAssetBuildData.h"
#include "Renderer/Scene/SceneLoadDesc.h"

#include <string>

namespace ElecNeko
{
    class AssetManager;

    struct SceneAssetBuilderOptions
    {
        bool importGeometryOnlyMeshes = true;
        bool importModelInstances = true;

        bool triangulate = true;
        bool generateNormals = false;
        bool generateTangents = false;
        bool flipUVs = false;

        bool mergeGeometryMeshes = true;
        bool deduplicateVertices = false;
    };

    struct SceneAssetBuilderResult
    {
        bool success = false;
        std::string errorMessage;

        SceneAssetBuildResult buildData;
    };

    class SceneAssetBuilder
    {
    public:
        static SceneAssetBuilderResult BuildSceneAssets(const SceneLoadDesc &sceneDesc, AssetManager &assetManager,
                                                        const SceneAssetBuilderOptions &options = SceneAssetBuilderOptions{});
    };
} // namespace ElecNeko
