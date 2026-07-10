// src/Renderer/Scene/SceneAssetBuildData.h
#pragma once

#include "Renderer/Assets/AssetHandle.h"
#include "Renderer/Scene/SceneLoadDesc.h"

#include <string>
#include <vector>

namespace ElecNeko
{
    struct SceneAssetMatrix
    {
        // Row-major 4x4 matrix.
        // This is CPU-side build data only.
        // The final conversion to engine Mat4 should happen in RenderSceneBuilder.
        float m[16] = {1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f};
    };

    struct SceneAssetMeshInstance
    {
        std::string name;

        StaticMeshHandle mesh;
        MaterialSetHandle materialSet;

        // Used by geometry-only scene mesh blocks.
        SceneTransformDesc transform;

        // Used by full imported model instances.
        // For now this stores the imported node-to-model matrix.
        // Root scene transform is still stored in transform above.
        SceneAssetMatrix localToModel;
        bool hasLocalToModel = false;

        bool visible = true;
    };

    struct SceneAssetBuildResult
    {
        std::vector<SceneAssetMeshInstance> meshInstances;

        void Clear() { meshInstances.clear(); }
    };
} // namespace ElecNeko
