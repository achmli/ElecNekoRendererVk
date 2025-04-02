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
        FrameBuffer::CreateParms_t createParams{};
        createParams.width = renderSize.x;
        createParams.height = renderSize.y;
        createParams.hasColor = true;
        createParams.hasDepth = false;
        return offscreenFrameBuffer->Create(device, createParams);
    }


} // namespace ElecNeko
