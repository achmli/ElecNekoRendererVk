//
// Created by ElecNekoSurface on 25-3-20.
//

#include "Renderer.h"

namespace ElecNeko
{
    Renderer::Renderer(std::unique_ptr<Scene> inScene) : scene(std::move(inScene))
    {
        if (!scene)
        {
            printf("No scene found\n");
            return;
        }
        if (!scene->initialized)
        {
            scene->ProcessScene();
        }

        offscreenFrameBuffer = std::make_shared<FrameBuffer>();

        renderSize = scene->renderOption.renderResolution;
        windowSize = scene->renderOption.windowResolution;
    }

    bool Renderer::InitializeFrameBuffer(DeviceContext *device) const
    {
        // frame buffer which render to screen at last
        {
            FrameBuffer::CreateParms_t createParams{};
            createParams.width = renderSize.x;
            createParams.height = renderSize.y;
            createParams.hasColor = true;
            createParams.hasDepth = false;
            if (!offscreenFrameBuffer->Create(device, createParams))
            {
                printf("Failed to create offscreen frame buffer\n");
                return false;
            }
        }

        // shadow map
        {
            FrameBuffer::CreateParms_t createParams{};
            createParams.width = 4096;
            createParams.height = 4096;
            createParams.hasColor = false;
            createParams.hasDepth = true;
            if (!shadowMapFrameBuffer->Create(device, createParams))
            {
                printf("Failed to create shadow map frame buffer\n");
                return false;
            }
        }

        return true;
    }

    bool Renderer::InitializeRenderPasses(DeviceContext *device)
    {
        int i = 0;
        // sky
        {
            renderPasses.emplace_back(offscreenFrameBuffer, false, false, 0, 0, 0, Pipeline::CULL_MODE_BACK, SkyPass, "sky");
            if (!renderPasses[i++].InitializePipeline(device))
            {
                printf("failed to initialize skybox pass\n");
                return false;
            }
        }

        // opaque shadow
        {
            renderPasses.emplace_back(shadowMapFrameBuffer, true, true, 2, 0, 0, Pipeline::CULL_MODE_FRONT, OpaqueShadowMapPass, "shadow2");
            if (!renderPasses[i++].InitializePipeline(device))
            {
                printf("failed to initialize opaque object shadow map pass\n");
                return false;
            }
        }

        // alpha test shadow
        {
            renderPasses.emplace_back(shadowMapFrameBuffer, true, true, 2, 1, 1, Pipeline::CULL_MODE_FRONT, AlphaTestShadowMapPass, "shadow2");
            if (!renderPasses[i++].InitializePipeline(device))
            {
                printf("failed to initialize alpha test object shadow map pass\n");
                return false;
            }
        }

        // opaque objects pass
        {
            renderPasses.emplace_back(offscreenFrameBuffer, true, true, 3, 1, 1, Pipeline::CULL_MODE_BACK, OpaquePass, "checkerboardShadowed2");
            if (!renderPasses[i++].InitializePipeline(device))
            {
                printf("failed to initialize opaque object pass\n");
                return false;
            }
        }

        {
            renderPasses.emplace_back(offscreenFrameBuffer, true, true, 3, 2, 2, Pipeline::CULL_MODE_BACK, AlphaTestPass, "loadedModelShadow");
            if (!renderPasses[i++].InitializePipeline(device))
            {
                printf("failed to initialize alpha test object pass\n");
                return false;
            }
        }

        return true;
    }

    void Renderer::CleanupOffScreen(DeviceContext *device)
    {
        for (auto &pass: renderPasses)
        {
            pass.Cleanup(device);
        }

        shadowMapFrameBuffer->Cleanup(device);
        offscreenFrameBuffer->Cleanup(device);
    }


} // namespace ElecNeko
