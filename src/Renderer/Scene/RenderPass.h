#pragma once
#include <memory>
#include <optional>
#include <utility>

#include "Renderer/FrameBuffer.h"
#include "Renderer/Pipeline.h"
#include "Renderer/shader.h"

namespace ElecNeko
{
    class RenderPass
    {
    public:
        RenderPass() = delete;
        RenderPass(const int wide, const int high, const bool beColor, const bool beDepth, const bool canDepthTest, const bool canDepthWrite,
                   const int vertUniformNum, const int fragUniformNum, const int samplerNum, const Pipeline::CullMode_t cull, const char *nameOfShader) :
            width(wide), height(high), hasColor(beColor), hasDepth(beDepth), enableDepthTest(canDepthTest), enableDepthWrite(canDepthWrite),
            vertexUniformNum(vertUniformNum), fragmentUniformNum(fragUniformNum), imageSamplerNum(samplerNum), cullMode(cull), shaderName(nameOfShader)
        {
            frameBuffer = std::make_shared<FrameBuffer>();
            frameBufferShared = false;
        }

        explicit RenderPass(const std::shared_ptr<FrameBuffer> &fb, const int wide, const int high, const bool beColor, const bool beDepth,
                            const bool canDepthTest, const bool canDepthWrite, const int vertUniformNum, const int fragUniformNum, const int samplerNum,
                            const Pipeline::CullMode_t cull, const char *nameOfShader) :
            frameBuffer(fb), width(wide), height(high), hasColor(beColor), hasDepth(beDepth), enableDepthTest(canDepthTest), enableDepthWrite(canDepthWrite),
            vertexUniformNum(vertUniformNum), fragmentUniformNum(fragUniformNum), imageSamplerNum(samplerNum), cullMode(cull), shaderName(nameOfShader)
        {
            frameBufferShared = true;
        }

        bool InitializeFrameBuffer(DeviceContext *device) const;

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
        bool hasColor;
        bool hasDepth;
        bool enableDepthTest;
        bool enableDepthWrite;
        bool frameBufferShared;

        int vertexUniformNum;
        int fragmentUniformNum;
        int imageSamplerNum;
        Pipeline::CullMode_t cullMode;

        const char *shaderName;
    };
} // namespace ElecNeko
