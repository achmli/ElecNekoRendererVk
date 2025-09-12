//
//  OffscreenRenderer.cpp
//
#include "OffscreenRenderer.h"
#include "Loader/Mesh.h"
#include "RenderOption.h"
#include "Samplers.h"
#include "model.h"

#include <assert.h>
#include <stdio.h>
#include <vector>
#include "../application.h"

FrameBuffer g_offscreenFrameBuffer;
Pipeline g_skyPipeline;
ElecNeko::ElecNekoShader g_skyShader;
Descriptors g_skyDescriptors;
Model g_skyModel;

Pipeline g_newSkyPipeline;
ElecNeko::ElecNekoShader g_newSkyShader;
Descriptors g_newSkyDescriptors;

// Pipeline	g_checkerboardShadowPipeline;
// Shader		g_checkerboardShadowShader;
// Descriptors	g_checkerboardShadowDescriptors;

FrameBuffer g_shadowFrameBuffer;
Pipeline g_shadowPipeline;
ElecNeko::ElecNekoShader g_shadowShader;
Descriptors g_shadowDescriptors;

Pipeline g_meshShadowPipeline;
ElecNeko::ElecNekoShader g_meshShadowShader;
Descriptors g_meshShadowDescriptors;

/*
====================================================
InitOffscreen
====================================================
*/
bool InitOffscreen(DeviceContext *device, int width, int height)
{
    bool result;

    //
    //	Build the frame buffer to render into
    //
    {
        FrameBuffer::CreateParms_t frameBufferParms;
        frameBufferParms.width = width;
        frameBufferParms.height = height;
        frameBufferParms.hasColor = true;
        frameBufferParms.hasDepth = true;
        result = g_offscreenFrameBuffer.Create(device, frameBufferParms);
        if (!result)
        {
            printf("ERROR: Failed to create off screen buffer\n");
            assert(0);
            return false;
        }
    }

    //
    //	Shadow
    //
    {
        FrameBuffer::CreateParms_t frameBufferParms;
        frameBufferParms.width = 4096;
        frameBufferParms.height = 4096;
        frameBufferParms.hasColor = false;
        frameBufferParms.hasDepth = true;
        result = g_shadowFrameBuffer.Create(device, frameBufferParms);
        if (!result)
        {
            printf("ERROR: Failed to create off screen buffer\n");
            assert(0);
            return false;
        }

        result = g_shadowShader.Load(device, "shadow2");
        if (!result)
        {
            printf("ERROR: Failed to load shader\n");
            assert(0);
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = 2;
        result = g_shadowDescriptors.Create(device, descriptorParms);
        if (!result)
        {
            printf("ERROR: Failed to build descriptors\n");
            assert(0);
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        pipelineParms.framebuffer = &g_shadowFrameBuffer;
        pipelineParms.descriptors = &g_shadowDescriptors;
        pipelineParms.shader = &g_shadowShader;
        pipelineParms.width = frameBufferParms.width;
        pipelineParms.height = frameBufferParms.height;
        pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
        pipelineParms.depthTest = true;
        pipelineParms.depthWrite = true;
        result = g_shadowPipeline.CreateForMesh(device, pipelineParms);
        if (!result)
        {
            printf("ERROR: Failed to build pipeline\n");
            assert(0);
            return false;
        }
    }

    //
    //	Sky
    //
    if (0)
    {
        result = g_skyShader.Load(device, "sky");
        if (!result)
        {
            printf("ERROR: Failed to load shader\n");
            assert(0);
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = 1;
        result = g_skyDescriptors.Create(device, descriptorParms);
        if (!result)
        {
            printf("ERROR: Failed to build descriptors\n");
            assert(0);
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        pipelineParms.framebuffer = &g_offscreenFrameBuffer;
        pipelineParms.descriptors = &g_skyDescriptors;
        pipelineParms.shader = &g_skyShader;
        pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
        pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
        pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
        pipelineParms.depthTest = false;
        pipelineParms.depthWrite = false;
        result = g_skyPipeline.Create(device, pipelineParms);
        if (!result)
        {
            printf("ERROR: Failed to build pipeline\n");
            assert(0);
            return false;
        }

        ShapeSphere sphereShape(1.0f);
        g_skyModel.BuildFromShape(&sphereShape);
        g_skyModel.MakeVBO(device);
    }

    {
        result = g_newSkyShader.Load(device, "newSky");
        if (!result)
        {
            printf("ERROR: Failed to load shader\n");
            assert(0);
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = 1;
        descriptorParms.numUniformsFragment = 1;
        descriptorParms.numImageSamplers = 1;
        result = g_newSkyDescriptors.Create(device, descriptorParms);
        if (!result)
        {
            printf("ERROR: Failed to build descriptors\n");
            assert(0);
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        pipelineParms.framebuffer = &g_offscreenFrameBuffer;
        pipelineParms.descriptors = &g_newSkyDescriptors;
        pipelineParms.shader = &g_newSkyShader;
        pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
        pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
        pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
        pipelineParms.depthTest = false;
        pipelineParms.depthWrite = false;
        result = g_newSkyPipeline.CreateForFullScreen(device, pipelineParms);
        if (!result)
        {
            printf("ERROR: Failed to build pipeline\n");
            assert(0);
            return false;
        }
    }

    //
    //	CheckerBoard Shadow
    //
    {
        /*result = g_checkerboardShadowShader.Load( device, "checkerboardShadowed2" );
        if ( !result ) {
            printf( "ERROR: Failed to load shader\n" );
            assert( 0 );
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset( &descriptorParms, 0, sizeof( descriptorParms ) );
        descriptorParms.numUniformsVertex = 3;
        descriptorParms.numUniformsFragment = 1;
        descriptorParms.numImageSamplers = 1;
        result = g_checkerboardShadowDescriptors.Create( device, descriptorParms );
        if ( !result ) {
            printf( "ERROR: Failed to build descriptors\n" );
            assert( 0 );
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        pipelineParms.framebuffer = &g_offscreenFrameBuffer;
        pipelineParms.descriptors = &g_checkerboardShadowDescriptors;
        pipelineParms.shader = &g_checkerboardShadowShader;
        pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
        pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
        pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
        pipelineParms.depthTest = true;
        pipelineParms.depthWrite = true;
        result = g_checkerboardShadowPipeline.Create( device, pipelineParms );
        if ( !result ) {
            printf( "ERROR: Failed to build pipeline\n" );
            assert( 0 );
            return false;
        }*/
    }

    {
        result = g_meshShadowShader.Load(device, "meshShadowed");
        if (!result)
        {
            printf("ERROR: Failed to load shader\n");
            assert(0);
            return false;
        }

        Descriptors::CreateParms_t descriptorParms;
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = 4;
        descriptorParms.numUniformsFragment = 2;
        descriptorParms.numImageSamplers = 2;
        result = g_meshShadowDescriptors.Create(device, descriptorParms);
        if (!result)
        {
            printf("ERROR: Failed to build descriptors\n");
            assert(0);
            return false;
        }

        Pipeline::CreateParms_t pipelineParms;
        pipelineParms.framebuffer = &g_offscreenFrameBuffer;
        pipelineParms.descriptors = &g_meshShadowDescriptors;
        pipelineParms.shader = &g_meshShadowShader;
        pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
        pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
        pipelineParms.cullMode = Pipeline::CULL_MODE_BACK;
        pipelineParms.depthTest = true;
        pipelineParms.depthWrite = true;

        result = g_meshShadowPipeline.CreateForMesh(device, pipelineParms);
        if (!result)
        {
            printf("ERROR: Failed to build pipeline\n");
            assert(0);
            return false;
        }
    }

    return true;
}

/*
====================================================
CleanupOffscreen
====================================================
*/
bool CleanupOffscreen(DeviceContext *device)
{
    if (0)
    {
        g_skyPipeline.Cleanup(device);
        g_skyDescriptors.Cleanup(device);
        g_skyShader.Cleanup(device);
        g_skyModel.Cleanup(*device);
    }
    g_offscreenFrameBuffer.Cleanup(device);

    /*g_checkerboardShadowPipeline.Cleanup( device );
    g_checkerboardShadowShader.Cleanup( device );
    g_checkerboardShadowDescriptors.Cleanup( device );*/

    g_newSkyPipeline.Cleanup(device);
    g_newSkyDescriptors.Cleanup(device);
    g_newSkyShader.Cleanup(device);

    g_meshShadowPipeline.Cleanup(device);
    g_meshShadowShader.Cleanup(device);
    g_meshShadowDescriptors.Cleanup(device);

    g_shadowPipeline.Cleanup(device);
    g_shadowShader.Cleanup(device);
    g_shadowDescriptors.Cleanup(device);
    g_shadowFrameBuffer.Cleanup(device);
    return true;
}

/*
====================================================
DrawOffscreen
====================================================
*/
void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, const RenderModel *renderModels, const int numModels)
{
    VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[cmdBufferIndex];

    const int camOffset = 0;
    const int camSize = sizeof(float) * 16 * 4;

    const int shadowCamOffset = device->GetAligendUniformByteOffset(camOffset + camSize);
    const int shadowCamSize = camSize;

    //
    //	Update the Shadows
    //
    {
        g_shadowFrameBuffer.m_imageDepth.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

        g_shadowFrameBuffer.BeginRenderPass(device, cmdBufferIndex);

        // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
        g_shadowPipeline.BindPipeline(cmdBuffer);
        for (int i = 0; i < numModels; i++)
        {
            const RenderModel &renderModel = renderModels[i];

            // Descriptor is how we bind our buffers and images
            Descriptor descriptor = g_shadowPipeline.GetFreeDescriptor();
            descriptor.BindBuffer(uniforms, shadowCamOffset, shadowCamSize, 0); // bind the camera matrices
            descriptor.BindBuffer(uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1); // bind the model matrices
            descriptor.BindDescriptor(device, cmdBuffer, &g_shadowPipeline);
            renderModel.model->DrawIndexed(cmdBuffer);
        }

        g_shadowFrameBuffer.EndRenderPass(device, cmdBufferIndex);

        g_shadowFrameBuffer.m_imageDepth.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
    }

    //
    //	Draw the World
    //
    {
        g_offscreenFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

        g_offscreenFrameBuffer.BeginRenderPass(device, cmdBufferIndex);

        //
        //	Draw the sky
        //
        {
            // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
            g_skyPipeline.BindPipeline(cmdBuffer);

            // Descriptor is how we bind our buffers and images
            Descriptor descriptor = g_skyPipeline.GetFreeDescriptor();
            descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
            descriptor.BindDescriptor(device, cmdBuffer, &g_skyPipeline);
            g_skyModel.DrawIndexed(cmdBuffer);
        }

        //
        //	Draw the models
        //
        {
            // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
            // g_checkerboardShadowPipeline.BindPipeline( cmdBuffer );
            // for ( int i = 0; i < numModels; i++ ) {
            //	const RenderModel & renderModel = renderModels[ i ];

            //	// Descriptor is how we bind our buffers and images
            //	Descriptor descriptor = g_checkerboardShadowPipeline.GetFreeDescriptor();
            //	descriptor.BindBuffer( uniforms, camOffset, camSize, 0 );									// bind the camera matrices
            //	descriptor.BindBuffer( uniforms, renderModel.uboByteOffset, renderModel.uboByteSize, 1 );	// bind the model matrices
            //	descriptor.BindBuffer( uniforms, shadowCamOffset, shadowCamSize, 2 );						// bind the shadow camera matrices
            //	descriptor.BindImage( VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBuffer.m_imageDepth.m_vkImageView,
            // Samplers::m_samplerStandard, 0 ); 	descriptor.BindDescriptor( device, cmdBuffer, &g_checkerboardShadowPipeline );
            // renderModel.model->DrawIndexed( cmdBuffer );
            //}
        }

        g_offscreenFrameBuffer.EndRenderPass(device, cmdBufferIndex);

        g_offscreenFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
    }
}

FrameBuffer g_postProcessFrameBuffer;

namespace ElecNeko
{
    ElecNekoPipeline g_skyPipelineEN;
    ElecNekoShader g_skyShaderEN;
    ElecNekoDescriptors g_skyDescriptorsEN;
    Model g_skyModelEN;

    ElecNekoPipeline g_newSkyPipelineEN;
    ElecNekoShader g_newSkyShaderEN;
    ElecNekoDescriptors g_newSkyDescriptorsEN;

    // Pipeline	g_checkerboardShadowPipeline;
    // Shader		g_checkerboardShadowShader;
    // Descriptors	g_checkerboardShadowDescriptors;

    FrameBuffer g_shadowFrameBufferEN;
    ElecNekoPipeline g_shadowPipelineEN;
    ElecNekoShader g_shadowShaderEN;
    ElecNekoDescriptors g_shadowDescriptorsEN;

    ElecNekoPipeline g_meshShadowPipelineEN;
    ElecNekoShader g_meshShadowShaderEN;
    ElecNekoDescriptors g_meshShadowDescriptorsEN;

    ElecNekoPipeline g_simpleSkyPipeline;
    ElecNekoShader g_simpleSkyShader;
    ElecNekoDescriptors g_simpleSkyDescriptors;

    ElecNekoPipeline g_tonemapPipeline;
    ElecNekoShader g_tonemapShader;
    ElecNekoDescriptors g_tonemapDescriptors;

    ElecNekoPipeline g_alphaTestShadowPipeline;
    ElecNekoShader g_alphaTestShadowShader;
    ElecNekoDescriptors g_alphaTestShadowDescriptors;

    ElecNekoPipeline g_alphaBlendMeshPipeline;
    ElecNekoShader g_alphaBlendMeshShader;
    ElecNekoDescriptors g_alphaBlendMeshDescriptors;

    ElecNekoPipeline g_alphaTestMeshPipeline;
    ElecNekoShader g_alphaTestMeshShader;
    ElecNekoDescriptors g_alphaTestMeshDescriptors;

    bool InitOffscreen(DeviceContext *device, const RenderOption &renderOption, int width, int height)
    {
        bool result;

        //
        //	Build the frame buffer to render into
        //
        {
            FrameBuffer::CreateParms_t frameBufferParms{};
            frameBufferParms.width = width;
            frameBufferParms.height = height;
            frameBufferParms.hasColor = true;
            frameBufferParms.hasDepth = true;
            result = g_offscreenFrameBuffer.Create(device, frameBufferParms);
            if (!result)
            {
                printf("ERROR: Failed to create off screen buffer\n");
                assert(0);
                return false;
            }
        }

        //
        //	Shadow
        //
        {
            FrameBuffer::CreateParms_t frameBufferParms{};
            frameBufferParms.width = 4096;
            frameBufferParms.height = 4096;
            frameBufferParms.hasColor = false;
            frameBufferParms.hasDepth = true;
            result = g_shadowFrameBufferEN.Create(device, frameBufferParms);
            if (!result)
            {
                printf("ERROR: Failed to create off screen buffer\n");
                assert(0);
                return false;
            }

            result = g_shadowShaderEN.Load(device, "shadowTest");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 0;
            descriptorParms.numStorageFragment = 0;
            descriptorParms.numImageSamplers = 0;
            result = g_shadowDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_shadowFrameBufferEN;
            pipelineParms.descriptors = &g_shadowDescriptorsEN;
            pipelineParms.shader = &g_shadowShaderEN;
            pipelineParms.width = frameBufferParms.width;
            pipelineParms.height = frameBufferParms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = true;
            pipelineParms.depthWrite = true;
            result = g_shadowPipelineEN.Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }

        // alpha test shadow map
        {
            result = g_alphaTestShadowShader.Load(device, "alphaTestShadow");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 0;
            descriptorParms.numStorageFragment = 1;
            descriptorParms.numImageSamplers = 1;
            result = g_alphaTestShadowDescriptors.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_shadowFrameBufferEN;
            pipelineParms.descriptors = &g_alphaTestShadowDescriptors;
            pipelineParms.shader = &g_alphaTestShadowShader;
            pipelineParms.width = g_shadowFrameBuffer.m_parms.width;
            pipelineParms.height = g_shadowFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = true;
            pipelineParms.depthWrite = true;
            result = g_alphaTestShadowPipeline.Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }

        //
        //	Sky
        //
        if (!renderOption.skyBox && !renderOption.simpleRealSky)
        {
            result = g_skyShaderEN.Load(device, "sky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            result = g_skyDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_skyDescriptorsEN;
            pipelineParms.shader = &g_skyShaderEN;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_skyPipelineEN.Create(device, pipelineParms);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }

            ShapeSphere sphereShape(1.0f);
            g_skyModelEN.BuildFromShape(&sphereShape);
            g_skyModelEN.MakeVBO(device);
        }
        else if (!renderOption.skyBox)
        {
            // maybe because of parameters is not right, so its not really real....
            result = g_simpleSkyShader.Load(device, "SecondSky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 0;
            descriptorParms.numUniformsFragment = 3;
            descriptorParms.numImageSamplers = 0;
            result = g_simpleSkyDescriptors.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_simpleSkyDescriptors;
            pipelineParms.shader = &g_simpleSkyShader;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_FRONT;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_simpleSkyPipeline.Create(device, pipelineParms, ElecNekoPipeline::USAGE_FULL_SCREEN);

            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }
        else
        {
            result = g_newSkyShaderEN.Load(device, "newSky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            descriptorParms.numUniformsFragment = 1;
            descriptorParms.numImageSamplers = 1;
            result = g_newSkyDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_newSkyDescriptorsEN;
            pipelineParms.shader = &g_newSkyShaderEN;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_newSkyPipelineEN.Create(device, pipelineParms, ElecNekoPipeline::USAGE_FULL_SCREEN);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }

        {
            result = g_meshShadowShaderEN.Load(device, "meshShadowed");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 2;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 1;
            descriptorParms.numStorageFragment = 1;
            descriptorParms.numImageSamplers = 2;
            result = g_meshShadowDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_meshShadowDescriptorsEN;
            pipelineParms.shader = &g_meshShadowShaderEN;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
            pipelineParms.depthTest = true;
            pipelineParms.depthWrite = true;

            result = g_meshShadowPipelineEN.Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }

        {
            result = g_alphaTestMeshShader.Load(device, "maskMesh");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 2;
            descriptorParms.numStorageVertex = 1;
            descriptorParms.numUniformsFragment = 1;
            descriptorParms.numStorageFragment = 1;
            descriptorParms.numImageSamplers = 2;
            result = g_alphaTestMeshDescriptors.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_alphaTestMeshDescriptors;
            pipelineParms.shader = &g_alphaTestMeshShader;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_BACK;
            pipelineParms.depthTest = true;
            pipelineParms.depthWrite = true;
            result = g_alphaTestMeshPipeline.Create(device, pipelineParms, ElecNekoPipeline::USAGE_MESH);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }

        /*{
            result = g_alphaBlendMeshShader.Load(device, "maskMesh");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            Descriptors::CreateParms_t descriptorParms;
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 3;
            descriptorParms.numUniformsFragment = 2;
            descriptorParms.numImageSamplers = 5;
            result = g_alphaBlendMeshDescriptors.ElecNekoCreate(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            Pipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_alphaBlendMeshDescriptors;
            pipelineParms.shader = &g_alphaBlendMeshShader;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = Pipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = true;
            pipelineParms.depthWrite = true;
            result = g_alphaTestMeshPipeline.CreateForTransparency(device, pipelineParms);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }*/

        {
            FrameBuffer::CreateParms_t frameBufferParms{};
            frameBufferParms.width = width;
            frameBufferParms.height = height;
            frameBufferParms.hasColor = true;
            frameBufferParms.hasDepth = false;
            result = g_postProcessFrameBuffer.Create(device, frameBufferParms);
            if (!result)
            {
                printf("ERROR: Failed to create off screen buffer\n");
                assert(0);
                return false;
            }

            // tonemap
            result = g_tonemapShader.Load(device, "tonemap");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms{};
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 0;
            descriptorParms.numUniformsFragment = 1;
            descriptorParms.numImageSamplers = 1;
            result = g_tonemapDescriptors.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_postProcessFrameBuffer;
            pipelineParms.descriptors = &g_tonemapDescriptors;
            pipelineParms.shader = &g_tonemapShader;
            pipelineParms.width = g_postProcessFrameBuffer.m_parms.width;
            pipelineParms.height = g_postProcessFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;

            result = g_tonemapPipeline.Create(device, pipelineParms, ElecNekoPipeline::USAGE_FULL_SCREEN);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }


        return true;
    }

    bool CleanupOffscreen(DeviceContext *device, const RenderOption &renderOption)
    {
        if (!renderOption.skyBox && !renderOption.simpleRealSky)
        {
            g_skyPipelineEN.Cleanup(device);
            g_skyDescriptorsEN.Cleanup(device);
            g_skyShaderEN.Cleanup(device);
            g_skyModelEN.Cleanup(*device);
        }
        else if (!renderOption.skyBox)
        {
            g_simpleSkyPipeline.Cleanup(device);
            g_simpleSkyDescriptors.Cleanup(device);
            g_simpleSkyShader.Cleanup(device);
        }
        else
        {
            g_newSkyPipelineEN.Cleanup(device);
            g_newSkyDescriptorsEN.Cleanup(device);
            g_newSkyShaderEN.Cleanup(device);
        }

        g_offscreenFrameBuffer.Cleanup(device);

        g_meshShadowPipelineEN.Cleanup(device);
        g_meshShadowShaderEN.Cleanup(device);
        g_meshShadowDescriptorsEN.Cleanup(device);

        g_alphaTestShadowPipeline.Cleanup(device);
        g_alphaTestShadowShader.Cleanup(device);
        g_alphaTestShadowDescriptors.Cleanup(device);

        g_alphaTestMeshPipeline.Cleanup(device);
        g_alphaTestMeshShader.Cleanup(device);
        g_alphaTestMeshDescriptors.Cleanup(device);

        g_shadowPipelineEN.Cleanup(device);
        g_shadowShaderEN.Cleanup(device);
        g_shadowDescriptorsEN.Cleanup(device);
        g_shadowFrameBufferEN.Cleanup(device);

        g_tonemapPipeline.Cleanup(device);
        g_tonemapShader.Cleanup(device);
        g_tonemapDescriptors.Cleanup(device);
        g_postProcessFrameBuffer.Cleanup(device);
        return true;
    }

    bool ReinitializeSky(DeviceContext *device, const RenderOption &renderOption)
    {
        if (!renderOption.skyBox && !renderOption.simpleRealSky)
        {
            g_skyPipelineEN.Cleanup(device);
            g_skyDescriptorsEN.Cleanup(device);
            g_skyShaderEN.Cleanup(device);
            g_skyModelEN.Cleanup(*device);
        }
        else if (!renderOption.skyBox)
        {
            g_simpleSkyPipeline.Cleanup(device);
            g_simpleSkyDescriptors.Cleanup(device);
            g_simpleSkyShader.Cleanup(device);
        }
        else
        {
            g_newSkyPipelineEN.Cleanup(device);
            g_newSkyDescriptorsEN.Cleanup(device);
            g_newSkyShaderEN.Cleanup(device);
        }

        bool result;

        if (!renderOption.skyBox && !renderOption.simpleRealSky)
        {
            result = g_skyShaderEN.Load(device, "sky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms;
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            result = g_skyDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_skyDescriptorsEN;
            pipelineParms.shader = &g_skyShaderEN;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_skyPipelineEN.Create(device, pipelineParms);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }

            ShapeSphere sphereShape(1.0f);
            g_skyModel.BuildFromShape(&sphereShape);
            g_skyModel.MakeVBO(device);
        }
        else if (!renderOption.skyBox)
        {
            result = g_simpleSkyShader.Load(device, "SecondSky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms;
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 0;
            descriptorParms.numUniformsFragment = 3;
            descriptorParms.numImageSamplers = 0;
            result = g_simpleSkyDescriptors.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_simpleSkyDescriptors;
            pipelineParms.shader = &g_simpleSkyShader;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_simpleSkyPipeline.Create(device, pipelineParms, ElecNekoPipeline::USAGE_FULL_SCREEN);

            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }
        else
        {
            result = g_newSkyShaderEN.Load(device, "newSky");
            if (!result)
            {
                printf("ERROR: Failed to load shader\n");
                assert(0);
                return false;
            }

            ElecNekoDescriptors::CreateParms_t descriptorParms;
            memset(&descriptorParms, 0, sizeof(descriptorParms));
            descriptorParms.numUniformsVertex = 1;
            descriptorParms.numUniformsFragment = 1;
            descriptorParms.numImageSamplers = 1;
            result = g_newSkyDescriptorsEN.Create(device, descriptorParms);
            if (!result)
            {
                printf("ERROR: Failed to build descriptors\n");
                assert(0);
                return false;
            }

            ElecNekoPipeline::CreateParms_t pipelineParms;
            pipelineParms.framebuffer = &g_offscreenFrameBuffer;
            pipelineParms.descriptors = &g_newSkyDescriptorsEN;
            pipelineParms.shader = &g_newSkyShaderEN;
            pipelineParms.width = g_offscreenFrameBuffer.m_parms.width;
            pipelineParms.height = g_offscreenFrameBuffer.m_parms.height;
            pipelineParms.cullMode = ElecNekoPipeline::CULL_MODE_NONE;
            pipelineParms.depthTest = false;
            pipelineParms.depthWrite = false;
            result = g_newSkyPipelineEN.Create(device, pipelineParms, ElecNekoPipeline::USAGE_FULL_SCREEN);
            if (!result)
            {
                printf("ERROR: Failed to build pipeline\n");
                assert(0);
                return false;
            }
        }
    }

    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox &skyBox, Scene *scene, const RenderOption &renderOption)
    {
        VkCommandBuffer cmdBuffer = device->m_vkCommandBuffers[cmdBufferIndex];

        const int camOffset = 0;
        const int camSize = sizeof(float) * 16 * 5;

        const int shadowCamOffset = device->GetAligendUniformByteOffset(camOffset + camSize);
        const int shadowCamSize = camSize;

        const int lightParmsOffset = device->GetAligendUniformByteOffset(shadowCamOffset + shadowCamSize);
        const int lightParmsSize = sizeof(float) * 16;

        const int skyParmsOffset = device->GetAligendUniformByteOffset(lightParmsOffset + lightParmsSize);
        const int skyParmsSize = sizeof(float) * 16;

        // update shadow
        {
            g_shadowFrameBufferEN.m_imageDepth.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            g_shadowFrameBufferEN.BeginRenderPass(device, cmdBufferIndex);

            if (!scene->opaqueVertices.empty())
            {
                g_shadowPipelineEN.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_shadowPipelineEN.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, shadowCamOffset, shadowCamSize, 0);
                // descriptor.BindBuffer(&scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize, 1);
                descriptor.BindUniformBuffer(0, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindStorageBuffer(1, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindDescriptor(device, cmdBuffer, &g_shadowPipelineEN);
                scene->DrawOpaqueIndexed(cmdBuffer);
            }

            if (!scene->maskVertices.empty())
            {
                g_alphaTestShadowPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_alphaTestShadowPipeline.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, shadowCamOffset, shadowCamSize, 0);
                // descriptor.BindBuffer(&scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize, 1);
                // descriptor.BindBuffer(&scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize, 2);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                //                      ElecNeko::ElecNekoSampler::m_samplerCubemap, 0);
                descriptor.BindUniformBuffer(0, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindStorageBuffer(1, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindStorageBuffer(2, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                descriptor.BindImage(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_alphaTestShadowPipeline);
                scene->DrawMaskIndexed(cmdBuffer);
            }

            g_shadowFrameBufferEN.EndRenderPass(device, cmdBufferIndex);
            g_shadowFrameBufferEN.m_imageDepth.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
        }

        {
            g_offscreenFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            g_offscreenFrameBuffer.BeginRenderPass(device, cmdBufferIndex);
            //
            //	Draw the sky
            //
            if (!renderOption.skyBox && !renderOption.simpleRealSky)
            {
                // Binding the pipeline is effectively the "use shader" we had back in our opengl apps
                g_skyPipelineEN.BindPipeline(cmdBuffer);

                // Descriptor is how we bind our buffers and images
                ElecNekoDescriptor descriptor = g_skyPipelineEN.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindDescriptor(device, cmdBuffer, &g_skyPipelineEN);
                g_skyModel.DrawIndexed(cmdBuffer);
            }
            else if (!renderOption.skyBox)
            {
                g_simpleSkyPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_simpleSkyPipeline.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
                // descriptor.BindBuffer(uniforms, lightParmsOffset, lightParmsSize, 1);
                // descriptor.BindBuffer(uniforms, skyParmsOffset, skyParmsSize, 2);
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindUniformBuffer(1, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindUniformBuffer(2, uniforms, skyParmsOffset, skyParmsSize);
                descriptor.BindDescriptor(device, cmdBuffer, &g_simpleSkyPipeline);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }
            else
            {
                g_newSkyPipelineEN.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_newSkyPipelineEN.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, skyBox.m_cubeImage.m_vkImageView, ElecNeko::ElecNekoSampler::m_samplerCubemap,
                //                      0);
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, skyBox.m_cubeImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_newSkyPipelineEN);
                vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
            }

            // draw the model
            if (!scene->opaqueVertices.empty())
            {
                g_meshShadowPipelineEN.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_meshShadowPipelineEN.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
                // descriptor.BindBuffer(&scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize, 1);
                // descriptor.BindBuffer(uniforms, shadowCamOffset, shadowCamSize, 2);
                // descriptor.BindBuffer(&scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize, 3);
                // descriptor.BindBuffer(uniforms, lightParmsOffset, lightParmsSize, 4);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                //                      Samplers::m_samplerStandard, 0);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                //                      ElecNeko::ElecNekoSampler::m_samplerCubemap, 1);
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindUniformBuffer(1, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindStorageBuffer(2, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindUniformBuffer(3, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindStorageBuffer(4, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                descriptor.BindImage(5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                                     Samplers::m_samplerStandard);
                descriptor.BindImage(6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_meshShadowPipelineEN);
                scene->DrawOpaqueIndexed(cmdBuffer);
            }
            if (!scene->maskVertices.empty())
            {
                g_alphaTestMeshPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_alphaTestMeshPipeline.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, camOffset, camSize, 0);
                // descriptor.BindBuffer(&scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize, 1);
                // descriptor.BindBuffer(uniforms, shadowCamOffset, shadowCamSize, 2);
                // descriptor.BindBuffer(&scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize, 3);
                // descriptor.BindBuffer(uniforms, lightParmsOffset, lightParmsSize, 4);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                //                      Samplers::m_samplerStandard, 0);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                //                      ElecNeko::ElecNekoSampler::m_samplerCubemap, 1);
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindUniformBuffer(1, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindStorageBuffer(2, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindUniformBuffer(3, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindStorageBuffer(4, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                descriptor.BindImage(5, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                                     Samplers::m_samplerStandard);
                descriptor.BindImage(6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_alphaTestMeshPipeline);
                scene->DrawMaskIndexed(cmdBuffer);
            }

            g_offscreenFrameBuffer.EndRenderPass(device, cmdBufferIndex);

            g_offscreenFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
        }

        // post process
        {
            g_postProcessFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            g_postProcessFrameBuffer.BeginRenderPass(device, cmdBufferIndex);

            // tonemapping
            {
                g_tonemapPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_tonemapPipeline.GetFreeDescriptor();
                // descriptor.BindBuffer(uniforms, lightParmsOffset, lightParmsSize, 0);
                // descriptor.BindImage(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_offscreenFrameBuffer.m_imageColor.m_vkImageView,
                //                      ElecNekoSampler::m_samplerTexture, 0);
                descriptor.BindUniformBuffer(0, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_offscreenFrameBuffer.m_imageColor.m_vkImageView,
                                     ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &g_tonemapPipeline);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }

            g_postProcessFrameBuffer.EndRenderPass(device, cmdBufferIndex);

            g_postProcessFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
} // namespace ElecNeko
