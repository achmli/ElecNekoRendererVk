//
// Created by ElecNekoSurface on 25-4-1.
//

#include "RenderPass.h"

namespace ElecNeko
{
    bool RenderPass::InitializeDescriptors(DeviceContext *device)
    {
        Descriptors::CreateParms_t createParams{};
        memset(&createParams, 0, sizeof(createParams));
        if (vertexUniformNum)
        {
            createParams.numUniformsVertex = vertexUniformNum;
        }
        if (fragmentUniformNum)
        {
            createParams.numUniformsFragment = fragmentUniformNum;
        }
        if (imageSamplerNum)
        {
            createParams.numImageSamplers = imageSamplerNum;
        }

        return descriptors.Create(device, createParams);
    }

    bool RenderPass::InitializePipeline(DeviceContext *device)
    {
        if (!shader.Load(device, shaderName))
        {
            printf("Failed to load %s shader\n", shaderName);
            return false;
        }

        if (!InitializeDescriptors(device))
        {
            printf("Failed to initialize descriptors");
            return false;
        }

        Pipeline::CreateParms_t createParams{};
        createParams.framebuffer = frameBuffer.get();
        createParams.descriptors = &descriptors;
        createParams.shader = &shader;
        createParams.width = frameBuffer->m_parms.width;
        createParams.height = frameBuffer->m_parms.height;
        createParams.cullMode = cullMode;
        createParams.depthTest = enableDepthTest;
        createParams.depthWrite = enableDepthWrite;

        if (!pipeline.Create(device, createParams))
        {
            printf("Failed to initialize pipeline");
            return false;
        }

        return true;
    }

    void RenderPass::Cleanup(DeviceContext *device)
    {
        pipeline.Cleanup(device);
        shader.Cleanup(device);
        descriptors.Cleanup(device);
        frameBuffer.reset();
    }

} // namespace ElecNeko
