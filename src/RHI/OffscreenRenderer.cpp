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


FrameBuffer g_postProcessFrameBuffer;
Image c_postProcessImage;

namespace ElecNeko
{
    FrameBuffer g_shadowFrameBufferEN;
    ElecNekoPipeline g_shadowPipelineEN;
    ElecNekoShader g_shadowShaderEN;
    ElecNekoDescriptors g_shadowDescriptorsEN;

    // deferred rendering
    GFrameBuffer g_geometryFrameBuffer;
    ElecNekoPipeline g_geometryOpaquePipeline;
    ElecNekoShader g_geometryOpaqueShader;
    ElecNekoDescriptors g_geometryOpaqueDescriptors;

    ElecNekoPipeline g_geometryMaskPipeline;
    ElecNekoShader g_geometryMaskShader;
    ElecNekoDescriptors g_geometryMaskDescriptors;

    ElecNekoPipeline g_geometrySkyPipeline;
    ElecNekoShader g_geometrySkyShader;
    ElecNekoDescriptors g_geometrySkyDescriptors;

    Image c_lightingImage;
    ElecNekoPipeline c_directLightPipeline;
    ElecNekoShader c_directLightShader;
    ElecNekoDescriptorsCompute c_directLightDescriptors;

    ElecNekoPipeline c_tonemapPipeline;
    ElecNekoShader c_tonemapShader;
    ElecNekoDescriptorsCompute c_tonemapDescriptors;

    // forward rendering
    ElecNekoPipeline g_skyPipelineEN;
    ElecNekoShader g_skyShaderEN;
    ElecNekoDescriptors g_skyDescriptorsEN;
    Model g_skyModelEN;

    ElecNekoPipeline g_newSkyPipelineEN;
    ElecNekoShader g_newSkyShaderEN;
    ElecNekoDescriptors g_newSkyDescriptorsEN;

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

    void CreateSingleFrameBuffer(DeviceContext *device, FrameBuffer &frameBuffer, int width, int height, bool hasColor = true, bool hasDepth = true)
    {
        FrameBuffer::CreateParms_t frameBufferParms{};
        frameBufferParms.width = width;
        frameBufferParms.height = height;
        frameBufferParms.hasColor = hasColor;
        frameBufferParms.hasDepth = hasDepth;
        if (!frameBuffer.Create(device, frameBufferParms))
        {
            printf("ERROR: Failed to create offscreen framebuffer\n");
            assert(0);
        }
    }

    void CreateGeometryFrameBuffer(DeviceContext *device, GFrameBuffer &frameBuffer, int width, int height)
    {
        GFrameBuffer::CreateParms_t frameBufferParms{};
        frameBufferParms.width = width;
        frameBufferParms.height = height;
        if (!frameBuffer.Create(device, frameBufferParms))
        {
            printf("ERROR: Failed to create geometry framebuffer\n");
            assert(0);
        }
    }

    void CreateComputeStorageImage(DeviceContext *device, Image &image, int width, int height, VkFormat format,
                                   VkImageUsageFlags usageFlags = VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT)
    {
        Image::CreateParms_t imageParms{};
        imageParms.width = width;
        imageParms.height = height;
        imageParms.depth = 1;
        imageParms.format = format;
        imageParms.usageFlags = usageFlags;
        if (!image.Create(device, imageParms))
        {
            printf("ERROR: Failed to create storage image\n");
            assert(0);
        }
        image.TransitionLayout(device);
    }

    void CreateGraphics(DeviceContext *device, const std::string &shaderName, ElecNekoPipeline &pipeline, ElecNekoDescriptors &descriptors,
                        ElecNekoShader &shader, FrameBuffer *framebuffer, ElecNekoPipeline::CullMode_t cullMode, bool depthTest, bool depthWrite,
                        ElecNekoPipeline::Usage_t usage, int numUniformsVertex, int numStorageVertex, int numUniformsFragment, int numStorageFragment,
                        int numImageSamplers)
    {
        if (!shader.Load(device, shaderName.c_str()))
        {
            std::cout << "Failed to Load Shader: " << shaderName << std::endl;
            assert(0);
            return;
        }

        // CreateGraphicsDescriptor(device, descriptors, numUniformsVertex, numStorageVertex, numUniformsFragment, numStorageFragment, numImageSamplers);
        ElecNekoDescriptors::CreateParms_t descriptorParms{};
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = numUniformsVertex;
        descriptorParms.numStorageVertex = numStorageVertex;
        descriptorParms.numUniformsFragment = numUniformsFragment;
        descriptorParms.numStorageFragment = numStorageFragment;
        descriptorParms.numImageSamplers = numImageSamplers;
        if (!descriptors.Create(device, descriptorParms))
        {
            std::cout << "Failed to Create Descriptor: " << shaderName << std::endl;
            assert(0);
            return;
        }

        ElecNekoPipeline::CreateParms_t pipelineParms{};
        pipelineParms.framebuffer = framebuffer;
        pipelineParms.descriptors = &descriptors;
        pipelineParms.shader = &shader;
        pipelineParms.width = framebuffer->m_parms.width;
        pipelineParms.height = framebuffer->m_parms.height;
        pipelineParms.cullMode = cullMode;
        pipelineParms.depthTest = depthTest;
        pipelineParms.depthWrite = depthWrite;
        if (!pipeline.Create(device, pipelineParms, usage))
        {
            std::cout << "Failed to Create Pipeline: " << shaderName << std::endl;
            assert(0);
        }
    }

    void CreateGeometry(DeviceContext *device, const std::string &shaderName, ElecNekoPipeline &pipeline, ElecNekoDescriptors &descriptors,
                        ElecNekoShader &shader, GFrameBuffer *gFramebuffer, ElecNekoPipeline::CullMode_t cullMode, bool depthTest, bool depthWrite,
                        int colorAttachmentCount, ElecNekoPipeline::Usage_t usage, int numUniformsVertex, int numStorageVertex, int numUniformsFragment,
                        int numStorageFragment, int numImageSamplers)
    {
        if (!shader.Load(device, shaderName.c_str()))
        {
            std::cout << "Failed to Load Shader: " << shaderName << std::endl;
            assert(0);
            return;
        }

        // CreateGraphicsDescriptor(device, descriptors, numUniformsVertex, numStorageVertex, numUniformsFragment, numStorageFragment, numImageSamplers);
        ElecNekoDescriptors::CreateParms_t descriptorParms{};
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniformsVertex = numUniformsVertex;
        descriptorParms.numStorageVertex = numStorageVertex;
        descriptorParms.numUniformsFragment = numUniformsFragment;
        descriptorParms.numStorageFragment = numStorageFragment;
        descriptorParms.numImageSamplers = numImageSamplers;
        if (!descriptors.Create(device, descriptorParms))
        {
            std::cout << "Failed to Create Descriptor: " << shaderName << std::endl;
            assert(0);
            return;
        }

        ElecNekoPipeline::CreateParms_t pipelineParms{};
        pipelineParms.gFramebuffer = gFramebuffer;
        pipelineParms.descriptors = &descriptors;
        pipelineParms.shader = &shader;
        pipelineParms.width = gFramebuffer->m_parms.width;
        pipelineParms.height = gFramebuffer->m_parms.height;
        pipelineParms.cullMode = cullMode;
        pipelineParms.depthTest = depthTest;
        pipelineParms.depthWrite = depthWrite;
        pipelineParms.colorAttachmentCount = colorAttachmentCount;
        if (!pipeline.Create(device, pipelineParms, usage))
        {
            std::cout << "Failed to Create Pipeline: " << shaderName << std::endl;
            assert(0);
        }
    }

    void CreateCompute(DeviceContext *device, const std::string &shaderName, ElecNekoPipeline &pipeline, ElecNekoDescriptorsCompute &descriptor,
                       ElecNekoShader &shader, int width, int height, int pushConstantSize, int numUniforms, int numStorageBuffers, int numStorageImages,
                       int numSampledImages, int maxSets)
    {
        if (!shader.Load(device, shaderName.c_str()))
        {
            std::cout << "Failed to Load Shader: " << shaderName << std::endl;
            assert(0);
            return;
        }

        ElecNekoDescriptorsCompute::CreateParms_t descriptorParms{};
        memset(&descriptorParms, 0, sizeof(descriptorParms));
        descriptorParms.numUniforms = numUniforms;
        descriptorParms.numStorageBuffers = numStorageBuffers;
        descriptorParms.numStorageImages = numStorageImages;
        descriptorParms.numSampledImages = numSampledImages;
        descriptorParms.maxSets = maxSets;
        if (!descriptor.Create(device, descriptorParms))
        {
            std::cout << "Failed to Create Descriptor: " << shaderName << std::endl;
            assert(0);
            return;
        }

        ElecNekoPipeline::CreateParms_t pipelineParms{};
        pipelineParms.descriptorsCompute = &descriptor;
        pipelineParms.shader = &shader;
        pipelineParms.width = width;
        pipelineParms.height = height;
        pipelineParms.pushConstantSize = pushConstantSize;
        if (!pipeline.CreateCompute(device, pipelineParms))
        {
            std::cout << "Failed to Create Pipeline: " << shaderName << std::endl;
            assert(0);
        }
    }

    bool InitOffscreen(DeviceContext *device, const RenderOption &renderOption, int width, int height)
    {

        //
        //	Shadow
        //
        CreateSingleFrameBuffer(device, g_shadowFrameBufferEN, 4096, 4096, false, true);
        CreateGraphics(device, "shadowTest", g_shadowPipelineEN, g_shadowDescriptorsEN, g_shadowShaderEN, &g_shadowFrameBufferEN,
                       ElecNekoPipeline::CULL_MODE_FRONT, true, true, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 0, 0);
        CreateGraphics(device, "alphaTestShadow", g_alphaTestShadowPipeline, g_alphaTestShadowDescriptors, g_alphaTestShadowShader, &g_shadowFrameBufferEN,
                       ElecNekoPipeline::CULL_MODE_FRONT, true, true, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 1, 1);

        // deferred pipeline
        {
            // Build G-Buffer
            CreateGeometryFrameBuffer(device, g_geometryFrameBuffer, width, height);

            // Sky for deferred rendering
            CreateGeometry(device, "geometryRealSky", g_geometrySkyPipeline, g_geometrySkyDescriptors, g_geometrySkyShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_NONE, false, false, 3, ElecNekoPipeline::USAGE_FULL_SCREEN, 0, 0, 3, 0, 0);

            // opaque mesh for deferred rendering
            CreateGeometry(device, "geometryOpaque", g_geometryOpaquePipeline, g_geometryOpaqueDescriptors, g_geometryOpaqueShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, 3, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 1, 1);

            // mask mesh
            CreateGeometry(device, "geometryMask", g_geometryMaskPipeline, g_geometryMaskDescriptors, g_geometryMaskShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, 3, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 1, 1);

            // lighting image for deferred rendering
            CreateComputeStorageImage(device, c_lightingImage, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateCompute(device, "directLight", c_directLightPipeline, c_directLightDescriptors, c_directLightShader, width, height, 0, 3, 0, 1, 5, 8);

            CreateComputeStorageImage(device, c_postProcessImage, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateCompute(device, "tonemapCompute", c_tonemapPipeline, c_tonemapDescriptors, c_tonemapShader, width, height, 0, 1, 0, 1, 1, 8);
        }

        // forward pipeline
        {
            //
            //	Build the frame buffer to render into for forward
            //
            CreateSingleFrameBuffer(device, g_offscreenFrameBuffer, width, height, true, true);

            //
            //	Sky
            //
            {
                CreateGraphics(device, "sky", g_skyPipelineEN, g_skyDescriptorsEN, g_skyShaderEN, &g_offscreenFrameBuffer, ElecNekoPipeline::CULL_MODE_NONE,
                               false, false, ElecNekoPipeline::USAGE_FULL_SCREEN, 1, 0, 0, 0, 0);

                ShapeSphere sphereShape(1.0f);
                g_skyModelEN.BuildFromShape(&sphereShape);
                g_skyModelEN.MakeVBO(device);
            }
            {
                CreateGraphics(device, "SecondSky", g_simpleSkyPipeline, g_simpleSkyDescriptors, g_simpleSkyShader, &g_offscreenFrameBuffer,
                               ElecNekoPipeline::CULL_MODE_FRONT, false, false, ElecNekoPipeline::USAGE_FULL_SCREEN, 0, 0, 3, 0, 0);
            }
            {
                CreateGraphics(device, "newSky", g_newSkyPipelineEN, g_newSkyDescriptorsEN, g_newSkyShaderEN, &g_offscreenFrameBuffer,
                               ElecNekoPipeline::CULL_MODE_NONE, false, false, ElecNekoPipeline::USAGE_FULL_SCREEN, 1, 0, 1, 0, 1);
            }

            CreateGraphics(device, "meshShadowed", g_meshShadowPipelineEN, g_meshShadowDescriptorsEN, g_meshShadowShaderEN, &g_offscreenFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, ElecNekoPipeline::USAGE_MESH, 2, 1, 1, 1, 2);

            CreateGraphics(device, "maskMesh", g_alphaTestMeshPipeline, g_alphaTestMeshDescriptors, g_alphaTestMeshShader, &g_offscreenFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, ElecNekoPipeline::USAGE_MESH, 2, 1, 1, 1, 2);

            CreateSingleFrameBuffer(device, g_postProcessFrameBuffer, width, height, true, false);
            CreateGraphics(device, "tonemap", g_tonemapPipeline, g_tonemapDescriptors, g_tonemapShader, &g_postProcessFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_NONE, false, false, ElecNekoPipeline::USAGE_FULL_SCREEN, 0, 0, 1, 1, 0);
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

        g_geometryFrameBuffer.Cleanup(device);
        g_geometryOpaquePipeline.Cleanup(device);
        g_geometryOpaqueShader.Cleanup(device);
        g_geometryOpaqueDescriptors.Cleanup(device);
        g_geometryMaskPipeline.Cleanup(device);
        g_geometryMaskShader.Cleanup(device);
        g_geometryMaskDescriptors.Cleanup(device);
        g_geometrySkyPipeline.Cleanup(device);
        g_geometrySkyShader.Cleanup(device);
        g_geometrySkyDescriptors.Cleanup(device);

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

        c_lightingImage.Cleanup(device);
        c_directLightPipeline.Cleanup(device);
        c_directLightDescriptors.Cleanup(device);
        c_directLightShader.Cleanup(device);

        c_postProcessImage.Cleanup(device);
        c_tonemapPipeline.Cleanup(device);
        c_tonemapDescriptors.Cleanup(device);
        c_tonemapShader.Cleanup(device);

        return true;
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
                descriptor.BindUniformBuffer(0, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindStorageBuffer(1, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindDescriptor(device, cmdBuffer, &g_shadowPipelineEN);
                scene->DrawOpaqueIndexed(cmdBuffer);
            }

            if (!scene->maskVertices.empty())
            {
                g_alphaTestShadowPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_alphaTestShadowPipeline.GetFreeDescriptor();
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

        // g-buffer pass
        if (renderOption.isDeferred)
        {
            g_geometryFrameBuffer.m_imageAlbedo.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageNormal.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageMaterial.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

            g_geometryFrameBuffer.BeginRenderPass(device, cmdBufferIndex);

            // Sky
            {
                // if (!renderOption.skyBox && !renderOption.simpleRealSky)
                // {
                //     g_geometrySkyPipeline.BindPipeline(cmdBuffer);
                //
                //     ElecNekoDescriptor descriptor = g_geometrySkyPipeline.GetFreeDescriptor();
                //     descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                //     descriptor.BindDescriptor(device, cmdBuffer, &g_geometrySkyPipeline);
                //     g_skyModelEN.DrawIndexed(cmdBuffer);
                // }
                // else if (renderOption.skyBox)
                // {
                //     g_geometrySkyPipeline.BindPipeline(cmdBuffer);
                //
                //     ElecNekoDescriptor descriptor = g_geometrySkyPipeline.GetFreeDescriptor();
                //     descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                //     descriptor.BindImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, skyBox.m_cubeImage.m_vkImageView,
                //                          ElecNeko::ElecNekoSampler::m_samplerCubemap);
                //     descriptor.BindDescriptor(device, cmdBuffer, &g_geometrySkyPipeline);
                //     vkCmdDraw(cmdBuffer, 36, 1, 0, 0);
                // }
                // else
                {
                    g_geometrySkyPipeline.BindPipeline(cmdBuffer);

                    ElecNekoDescriptor descriptor = g_geometrySkyPipeline.GetFreeDescriptor();
                    descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                    descriptor.BindUniformBuffer(1, uniforms, lightParmsOffset, lightParmsSize);
                    descriptor.BindUniformBuffer(2, uniforms, skyParmsOffset, skyParmsSize);
                    descriptor.BindDescriptor(device, cmdBuffer, &g_geometrySkyPipeline);
                    vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
                }
            }

            // opaque meshes
            if (!scene->opaqueVertices.empty())
            {
                g_geometryOpaquePipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_geometryOpaquePipeline.GetFreeDescriptor();
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindStorageBuffer(1, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindStorageBuffer(2, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                descriptor.BindImage(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_geometryOpaquePipeline);
                scene->DrawOpaqueIndexed(cmdBuffer);
            }
            if (!scene->maskVertices.empty()) // mask meshes
            {
                g_geometryMaskPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_geometryMaskPipeline.GetFreeDescriptor();
                descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                descriptor.BindStorageBuffer(1, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                descriptor.BindStorageBuffer(2, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                descriptor.BindImage(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                     ElecNeko::ElecNekoSampler::m_samplerCubemap);
                descriptor.BindDescriptor(device, cmdBuffer, &g_geometryMaskPipeline);
                scene->DrawMaskIndexed(cmdBuffer);
            }

            g_geometryFrameBuffer.EndRenderPass(device, cmdBufferIndex);

            g_geometryFrameBuffer.m_imageAlbedo.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            g_geometryFrameBuffer.m_imageNormal.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            g_geometryFrameBuffer.m_imageMaterial.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            g_geometryFrameBuffer.m_imageDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            c_lightingImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

            {
                c_directLightPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_directLightPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, camOffset, camSize);
                descriptor.BindingUniform(1, uniforms, shadowCamOffset, shadowCamSize);
                descriptor.BindingUniform(2, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindingStorageImage(3, VK_IMAGE_LAYOUT_GENERAL, c_lightingImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageAlbedo.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageMaterial.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(7, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(8, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerShadow);
                descriptor.BindDescriptor(device, cmdBuffer, &c_directLightPipeline);
                int groupX = (c_lightingImage.m_parms.width + 15) / 16;
                int groupY = (c_lightingImage.m_parms.height + 15) / 16;
                c_directLightPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
            }

            c_lightingImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            c_postProcessImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
            {
                c_tonemapPipeline.BindPipelineCompute(cmdBuffer);

                ElecNekoDescriptorCompute descriptor = c_tonemapPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindingStorageImage(1, VK_IMAGE_LAYOUT_GENERAL, c_postProcessImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_lightingImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &c_tonemapPipeline);
                int groupX = (c_postProcessImage.m_parms.width + 15) / 16;
                int groupY = (c_postProcessImage.m_parms.height + 15) / 16;
                c_tonemapPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
            }
        }
        else
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
                g_skyModelEN.DrawIndexed(cmdBuffer);
            }
            else if (renderOption.simpleRealSky)
            {
                g_simpleSkyPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_simpleSkyPipeline.GetFreeDescriptor();
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

            g_postProcessFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);

            g_postProcessFrameBuffer.BeginRenderPass(device, cmdBufferIndex);

            // tonemapping
            {
                g_tonemapPipeline.BindPipeline(cmdBuffer);

                ElecNekoDescriptor descriptor = g_tonemapPipeline.GetFreeDescriptor();
                descriptor.BindUniformBuffer(0, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_offscreenFrameBuffer.m_imageColor.m_vkImageView,
                                     ElecNekoSampler::m_samplerTexture);
                // descriptor.BindImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_lightingImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &g_tonemapPipeline);
                vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
            }

            g_postProcessFrameBuffer.EndRenderPass(device, cmdBufferIndex);

            g_postProcessFrameBuffer.m_imageColor.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
        }
    }
} // namespace ElecNeko
