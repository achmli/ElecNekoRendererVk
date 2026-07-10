#pragma once

#include "Renderer/Scene/SceneAssetBuildData.h"

#include <memory>
#include <string>

namespace RHI
{
    class Device;
}

namespace ElecNeko
{
    class AssetManager;
    class RenderScene;

    struct RenderSceneBuilderOptions
    {
        bool enableTextures = false;
        bool uploadGpuSceneBuffers = true;
    };

    struct RenderSceneBuilderResult
    {
        bool success = false;
        std::string errorMessage;

        std::unique_ptr<RenderScene> renderScene;
    };

    class RenderSceneBuilder
    {
    public:
        static RenderSceneBuilderResult BuildRenderScene(RHI::Device *rhiDevice, const SceneAssetBuildResult &buildData, AssetManager &assetManager,
                                                         const RenderSceneBuilderOptions &options);
    };
} // namespace ElecNeko
