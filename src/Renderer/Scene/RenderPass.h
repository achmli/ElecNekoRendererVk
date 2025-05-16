#pragma once
#include <memory>
#include <optional>
#include <utility>

#include "Renderer/FrameBuffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/shader.h"

namespace ElecNeko
{
    enum RenderPassMode
    {
        SkyPass,
        OpaqueShadowMapPass,
        // need another pass to write alpha test
        AlphaTestShadowMapPass,
        OpaquePass,
        AlphaTestPass,
        AlphaBlendPass,
        PostProcessPass
    };

    class RenderPass
    {
    public:
        RenderPass() = delete;

        explicit RenderPass(const std::shared_ptr<FrameBuffer> &fb, const bool canDepthTest, const bool canDepthWrite, const int vertUniformNum,
                            const int fragUniformNum, const int samplerNum, const Pipeline::CullMode_t cull, const RenderPassMode renderPassMode,
                            const char *nameOfShader) :
            frameBuffer(fb), width(fb->m_parms.width), height(fb->m_parms.height), cullMode(cull), passMode(renderPassMode), enableDepthTest(canDepthTest),
            enableDepthWrite(canDepthWrite), vertexUniformNum(vertUniformNum), fragmentUniformNum(fragUniformNum), imageSamplerNum(samplerNum),
            shaderName(nameOfShader)
        {}

        bool InitializeDescriptors(DeviceContext *device);

        bool InitializePipeline(DeviceContext *device);

        void Cleanup(DeviceContext *device);

    public:
        std::shared_ptr<FrameBuffer> frameBuffer;
        Pipeline pipeline;
        Shader shader;
        Descriptors descriptors;

        int width;
        int height;

        Pipeline::CullMode_t cullMode;
        RenderPassMode passMode;

        bool enableDepthTest;
        bool enableDepthWrite;

        int vertexUniformNum;
        int fragmentUniformNum;
        int imageSamplerNum;

        const char *shaderName;
    };
} // namespace ElecNeko
