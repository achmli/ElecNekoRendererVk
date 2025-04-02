#pragma once

#include <memory>


#include "RenderPass.h"
#include "Renderer/FrameBuffer.h"
#include "World.h"

namespace ElecNeko
{
    class Renderer
    {
    protected:
        std::unique_ptr<Scene> scene;

        iVec2 renderSize;
        iVec2 windowSize;

        std::shared_ptr<FrameBuffer> offscreenFrameBuffer;

    public:
        explicit Renderer(std::unique_ptr<Scene> inScene);

        // temporary: init offscreen frame buffer
        bool InitializeFrameBuffer(DeviceContext *device) const;

        // init render passes in order
        bool InitializeRenderPasses(DeviceContext *device);
    };
} // namespace ElecNeko
