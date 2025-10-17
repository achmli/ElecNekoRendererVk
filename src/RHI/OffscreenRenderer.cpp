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
#include "../../thirdParty/assimp/contrib/openddlparser/include/openddlparser/OpenDDLCommon.h"
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

    CubeImage c_environmentCubemap;
    ElecNekoPipeline c_environmentToCubemapPipeline;
    ElecNekoShader c_environmentToCubemapShader;
    ElecNekoDescriptorsCompute c_environmentToCubemapDescriptors;

    ElecNekoPipeline c_iblBlurPipeline;
    ElecNekoShader c_iblBlurShader;
    ElecNekoDescriptorsCompute c_iblBlurDescriptors;

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

    std::vector<Image> c_minMaxDepthImages;
    ElecNekoPipeline c_minMaxDepthPipeline;
    ElecNekoShader c_minMaxDepthShader;
    ElecNekoDescriptorsCompute c_minMaxDepthDescriptors;

    ElecNekoPipeline c_minMaxPipeline;
    ElecNekoShader c_minMaxShader;
    ElecNekoDescriptorsCompute c_minMaxDescriptors;

    Image c_ambientOcclusionRawImage;
    ElecNekoPipeline c_ambientOcclusionRawPipeline;
    ElecNekoShader c_ambientOcclusionRawShader;
    ElecNekoDescriptorsCompute c_ambientOcclusionRawDescriptors;

    Image c_ambientOcclusionBlurImage;
    ElecNekoPipeline c_ambientOcclusionBlurHPipeline;
    ElecNekoShader c_ambientOcclusionBlurHShader;
    ElecNekoDescriptorsCompute c_ambientOcclusionBlurHDescriptors;

    ElecNekoPipeline c_ambientOcclusionBlurVPipeline;
    ElecNekoShader c_ambientOcclusionBlurVShader;
    ElecNekoDescriptorsCompute c_ambientOcclusionBlurVDescriptors;

    Image c_lightingImage;
    Image c_specularLightingImage;
    ElecNekoPipeline c_directLightPipeline;
    ElecNekoShader c_directLightShader;
    ElecNekoDescriptorsCompute c_directLightDescriptors;

    Image c_ssrImage;
    ElecNekoPipeline c_ssrPipeline;
    ElecNekoShader c_ssrShader;
    ElecNekoDescriptorsCompute c_ssrDescriptors;

    Image c_compositeImage;
    ElecNekoPipeline c_compositePipeline;
    ElecNekoShader c_compositeShader;
    ElecNekoDescriptorsCompute c_compositeDescriptors;

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

    int ComputeSampleCountForMip(int baseSamples, int mip, int mipCount)
    {
        // simple geometric reduction: baseSamples >> mip, but clamp
        int s = std::max(8, baseSamples >> mip); // min 8
        return s;
    }

    int ClacReductionLevels(int width, int height, int step)
    {
        int levels = 0;
        while (width > 1 || height > 1)
        {
            width = (width + step - 1) / step;
            height = (height + step - 1) / step;
            levels++;
        }
        return levels;
    }

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

    struct IBLBlurPushConstant_t
    {
        uint32_t width;
        uint32_t height;
        float roughness;
        uint32_t sampleCount;
        uint32_t inputMip;
    };

    struct GTAOPushConstant_t
    {
        uint32_t width;
        uint32_t height;
        float radius;
        uint32_t sampleCount;
        float bias;
        float intensity;
        float maxDistance;
    };

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

        // IBL
        {
            int mipLevels = static_cast<int>(std::floor(std::log2(512))) + 1;
            c_environmentCubemap.Create(device, 512, 512, mipLevels, VK_FORMAT_R16G16B16A16_SFLOAT);

            CreateCompute(device, "environmentToCubemap", c_environmentToCubemapPipeline, c_environmentToCubemapDescriptors, c_environmentToCubemapShader, 512,
                          512, 0, 2, 0, 1, 0, 8);

            CreateCompute(device, "iblBlur", c_iblBlurPipeline, c_iblBlurDescriptors, c_iblBlurShader, 512, 512, sizeof(IBLBlurPushConstant_t), 0, 0, 1, 1, 16);

            ElecNekoSampler::InitializeIBLSampler(device, static_cast<float>(mipLevels - 1));
        }

        // ao
        {

            // ao raw
            CreateComputeStorageImage(device, c_ambientOcclusionRawImage, width / 2, height / 2, VK_FORMAT_R16_SFLOAT,
                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                              VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            CreateCompute(device, "gtaoRaw", c_ambientOcclusionRawPipeline, c_ambientOcclusionRawDescriptors, c_ambientOcclusionRawShader, width / 2,
                          height / 2, sizeof(GTAOPushConstant_t), 1, 0, 1, 2, 8);

            CreateComputeStorageImage(device, c_ambientOcclusionBlurImage, width / 2, height / 2, VK_FORMAT_R16_SFLOAT,
                                      VK_IMAGE_USAGE_STORAGE_BIT | VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT |
                                              VK_IMAGE_USAGE_TRANSFER_DST_BIT);
            CreateCompute(device, "gtaoBlurH", c_ambientOcclusionBlurHPipeline, c_ambientOcclusionBlurHDescriptors, c_ambientOcclusionBlurHShader, width / 2,
                          height / 2, 6 * sizeof(float), 0, 0, 1, 3, 8);

            CreateCompute(device, "gtaoBlurV", c_ambientOcclusionBlurVPipeline, c_ambientOcclusionBlurVDescriptors, c_ambientOcclusionBlurVShader, width / 2,
                          height / 2, 6 * sizeof(float), 0, 0, 1, 3, 8);
        }

        // deferred pipeline
        {
            // Build G-Buffer
            CreateGeometryFrameBuffer(device, g_geometryFrameBuffer, width, height);

            // Sky for deferred rendering
            CreateGeometry(device, "geometryRealSky", g_geometrySkyPipeline, g_geometrySkyDescriptors, g_geometrySkyShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_NONE, false, false, 3, ElecNekoPipeline::USAGE_FULL_SCREEN, 0, 0, 3, 0, 0);

            // opaque mesh for deferred rendering
            CreateGeometry(device, "geometryOpaque", g_geometryOpaquePipeline, g_geometryOpaqueDescriptors, g_geometryOpaqueShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, 4, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 1, 1);

            // mask mesh
            CreateGeometry(device, "geometryMask", g_geometryMaskPipeline, g_geometryMaskDescriptors, g_geometryMaskShader, &g_geometryFrameBuffer,
                           ElecNekoPipeline::CULL_MODE_BACK, true, true, 4, ElecNekoPipeline::USAGE_MESH, 1, 1, 0, 1, 1);

            {
                int numLevels = ClacReductionLevels(width, height, 16);

                c_minMaxDepthImages.resize(numLevels);
                int imageWidth = (width + 15) / 16;
                int imageHeight = (height + 15) / 16;
                for (auto &image: c_minMaxDepthImages)
                {
                    CreateComputeStorageImage(device, image, imageWidth, imageHeight, VK_FORMAT_R32G32_SFLOAT);
                    imageWidth = (imageWidth + 15) / 16;
                    imageHeight = (imageHeight + 15) / 16;
                }
                CreateCompute(device, "minMaxDepth", c_minMaxDepthPipeline, c_minMaxDepthDescriptors, c_minMaxDepthShader, 1, 1, 0, 0, 0, 1, 1, 8);
                CreateCompute(device, "minMax", c_minMaxPipeline, c_minMaxDescriptors, c_minMaxShader, 1, 1, 0, 0, 0, 1, 7, 8);
            }

            // lighting image for deferred rendering
            CreateComputeStorageImage(device, c_lightingImage, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateComputeStorageImage(device, c_specularLightingImage, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateCompute(device, "directLight", c_directLightPipeline, c_directLightDescriptors, c_directLightShader, width, height, 2 * sizeof(int), 5, 0, 2,
                          11, 8);

            // ssr
            CreateComputeStorageImage(device, c_ssrImage, width / 2, height / 2, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateCompute(device, "ssr", c_ssrPipeline, c_ssrDescriptors, c_ssrShader, width / 2, height / 2, 9 * sizeof(float), 1, 0, 1, 6, 8);

            // composite
            CreateComputeStorageImage(device, c_compositeImage, width, height, VK_FORMAT_R16G16B16A16_SFLOAT);
            CreateCompute(device, "composite", c_compositePipeline, c_compositeDescriptors, c_compositeShader, width, height, 6 * sizeof(float), 1, 0, 3, 5, 8);

            // post process
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
        c_specularLightingImage.Cleanup(device);
        c_directLightPipeline.Cleanup(device);
        c_directLightDescriptors.Cleanup(device);
        c_directLightShader.Cleanup(device);

        c_postProcessImage.Cleanup(device);
        c_tonemapPipeline.Cleanup(device);
        c_tonemapDescriptors.Cleanup(device);
        c_tonemapShader.Cleanup(device);

        c_environmentCubemap.Cleanup(device);
        c_environmentToCubemapPipeline.Cleanup(device);
        c_environmentToCubemapDescriptors.Cleanup(device);
        c_environmentToCubemapShader.Cleanup(device);
        c_iblBlurPipeline.Cleanup(device);
        c_iblBlurDescriptors.Cleanup(device);
        c_iblBlurShader.Cleanup(device);

        c_ambientOcclusionRawImage.Cleanup(device);
        c_ambientOcclusionRawPipeline.Cleanup(device);
        c_ambientOcclusionRawDescriptors.Cleanup(device);
        c_ambientOcclusionRawShader.Cleanup(device);

        c_ambientOcclusionBlurImage.Cleanup(device);
        c_ambientOcclusionBlurHPipeline.Cleanup(device);
        c_ambientOcclusionBlurHDescriptors.Cleanup(device);
        c_ambientOcclusionBlurHShader.Cleanup(device);

        c_ambientOcclusionBlurVPipeline.Cleanup(device);
        c_ambientOcclusionBlurVDescriptors.Cleanup(device);
        c_ambientOcclusionBlurVShader.Cleanup(device);

        for (auto &image: c_minMaxDepthImages)
        {
            image.Cleanup(device);
        }
        c_minMaxDepthImages.clear();
        c_minMaxDepthPipeline.Cleanup(device);
        c_minMaxDepthDescriptors.Cleanup(device);
        c_minMaxDepthShader.Cleanup(device);

        c_minMaxPipeline.Cleanup(device);
        c_minMaxDescriptors.Cleanup(device);
        c_minMaxShader.Cleanup(device);

        c_ssrImage.Cleanup(device);
        c_ssrPipeline.Cleanup(device);
        c_ssrDescriptors.Cleanup(device);
        c_ssrShader.Cleanup(device);

        c_compositeImage.Cleanup(device);
        c_compositePipeline.Cleanup(device);
        c_compositeDescriptors.Cleanup(device);
        c_compositeShader.Cleanup(device);

        return true;
    }

    void DrawOffscreen(DeviceContext *device, int cmdBufferIndex, Buffer *uniforms, SkyBox &skyBox, Scene *scene, RenderOption &renderOption,
                       CascadeShadow &csm)
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

        if (renderOption.isSkyChanged)
        {
            // sky cubemap
            c_environmentCubemap.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

            c_environmentCubemap.TransitionMipLayout(cmdBuffer, 0, VK_IMAGE_LAYOUT_GENERAL);

            c_environmentToCubemapPipeline.BindPipelineCompute(cmdBuffer);
            ElecNekoDescriptorCompute descriptor = c_environmentToCubemapPipeline.GetFreeDescriptorCompute();

            descriptor.BindingUniform(0, uniforms, lightParmsOffset, lightParmsSize);
            descriptor.BindingUniform(1, uniforms, skyParmsOffset, skyParmsSize);
            descriptor.BindingStorageImage(2, VK_IMAGE_LAYOUT_GENERAL, c_environmentCubemap.m_faceMipViews[0], VK_NULL_HANDLE);
            descriptor.BindDescriptor(device, cmdBuffer, &c_environmentToCubemapPipeline);
            c_environmentToCubemapPipeline.DispatchCompute(cmdBuffer, 512 / 16, 512 / 16, 6);
            c_environmentCubemap.TransitionMipLayout(cmdBuffer, 0, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            c_iblBlurPipeline.BindPipelineCompute(cmdBuffer);

            for (int i = 1; i < c_environmentCubemap.m_mipLevels; i++)
            {
                c_environmentCubemap.TransitionMipLayout(cmdBuffer, i, VK_IMAGE_LAYOUT_GENERAL);

                ElecNekoDescriptorCompute descriptorIBLBlur = c_iblBlurPipeline.GetFreeDescriptorCompute();

                descriptorIBLBlur.BindingStorageImage(0, VK_IMAGE_LAYOUT_GENERAL, c_environmentCubemap.m_faceMipViews[i], VK_NULL_HANDLE);
                descriptorIBLBlur.BindingSampledImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_environmentCubemap.m_faceMipViews[i - 1],
                                                      ElecNekoSampler::m_samplerIBL);
                descriptorIBLBlur.BindDescriptor(device, cmdBuffer, &c_iblBlurPipeline);
                IBLBlurPushConstant_t pushConstant;
                pushConstant.width = std::max(512 >> i, 1);
                pushConstant.height = pushConstant.width;
                pushConstant.roughness = float(i) / float(c_environmentCubemap.m_mipLevels - 1);
                pushConstant.sampleCount = ComputeSampleCountForMip(256, i, c_environmentCubemap.m_faceMipViews.size());
                pushConstant.inputMip = i - 1;
                c_iblBlurPipeline.PushConstants(cmdBuffer, &pushConstant, sizeof(IBLBlurPushConstant_t));
                c_iblBlurPipeline.DispatchCompute(cmdBuffer, (pushConstant.width + 15) / 16, (pushConstant.height + 15) / 16, 6);

                c_environmentCubemap.TransitionMipLayout(cmdBuffer, i, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            c_environmentCubemap.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            renderOption.isSkyChanged = false;
        }

        // g-buffer pass
        if (renderOption.isDeferred)
        {
            g_geometryFrameBuffer.m_imageAlbedo.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageNormal.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageMaterial.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
            g_geometryFrameBuffer.m_imageLinearDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL);
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
                    // g_geometrySkyPipeline.BindPipeline(cmdBuffer);
                    //
                    // ElecNekoDescriptor descriptor = g_geometrySkyPipeline.GetFreeDescriptor();
                    // descriptor.BindUniformBuffer(0, uniforms, camOffset, camSize);
                    // descriptor.BindUniformBuffer(1, uniforms, lightParmsOffset, lightParmsSize);
                    // descriptor.BindUniformBuffer(2, uniforms, skyParmsOffset, skyParmsSize);
                    // descriptor.BindDescriptor(device, cmdBuffer, &g_geometrySkyPipeline);
                    // vkCmdDraw(cmdBuffer, 3, 1, 0, 0);
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
            g_geometryFrameBuffer.m_imageLinearDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            g_geometryFrameBuffer.m_imageDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);

            {
                {
                    c_minMaxDepthPipeline.BindPipelineCompute(cmdBuffer);
                    c_minMaxDepthImages[0].TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
                    ElecNekoDescriptorCompute descriptor = c_minMaxDepthPipeline.GetFreeDescriptorCompute();
                    descriptor.BindingStorageImage(0, VK_IMAGE_LAYOUT_GENERAL, c_minMaxDepthImages[0].m_vkImageView, VK_NULL_HANDLE);
                    descriptor.BindingSampledImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageLinearDepth.m_vkImageView,
                                                   ElecNekoSampler::m_samplerTexture);
                    descriptor.BindDescriptor(device, cmdBuffer, &c_minMaxPipeline);
                    int groupX = (g_geometryFrameBuffer.m_parms.width + 15) / 16;
                    int groupY = (g_geometryFrameBuffer.m_parms.height + 15) / 16;
                    c_minMaxDepthPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
                    c_minMaxDepthImages[0].TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }

                c_minMaxPipeline.BindPipelineCompute(cmdBuffer);
                for (int i = 1; i < c_minMaxDepthImages.size(); i++)
                {
                    c_minMaxDepthImages[i].TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
                    ElecNekoDescriptorCompute descriptor = c_minMaxPipeline.GetFreeDescriptorCompute();
                    descriptor.BindingStorageImage(0, VK_IMAGE_LAYOUT_GENERAL, c_minMaxDepthImages[i].m_vkImageView, VK_NULL_HANDLE);
                    descriptor.BindingSampledImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_minMaxDepthImages[i - 1].m_vkImageView,
                                                   ElecNekoSampler::m_samplerTexture);
                    descriptor.BindDescriptor(device, cmdBuffer, &c_minMaxPipeline);
                    int groupX = (c_minMaxDepthImages[i - 1].m_parms.width + 15) / 16;
                    int groupY = (c_minMaxDepthImages[i - 1].m_parms.height + 15) / 16;
                    c_minMaxPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
                    c_minMaxDepthImages[i].TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
                }
            }

            float minMaxDepth[2];
            c_minMaxDepthImages[c_minMaxDepthImages.size() - 1].ReadPixelRGToCPU(device, minMaxDepth);
            csm.UpdateDistance(minMaxDepth[0], minMaxDepth[1]);
            csm.Update(renderOption.sunDirection,
                       static_cast<float>(g_geometryFrameBuffer.m_parms.height) / static_cast<float>(g_geometryFrameBuffer.m_parms.width));
            csm.MakeUBO(device);

            {
                for (int i = 0; i < csm.numCascade; i++)
                {
                    csm.m_shadowMaps[i].m_imageDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);

                    csm.m_shadowMaps[i].BeginRenderPass(device, cmdBufferIndex);

                    if (!scene->opaqueVertices.empty())
                    {
                        csm.m_pipelines[i].BindPipeline(cmdBuffer);

                        ElecNekoDescriptor descriptor = csm.m_pipelines[i].GetFreeDescriptor();
                        descriptor.BindUniformBuffer(0, &csm.m_viewMatrixBuffers, i * sizeof(Mat4), sizeof(Mat4));
                        descriptor.BindUniformBuffer(1, &csm.m_projMatrixBuffers, i * sizeof(Mat4), sizeof(Mat4));
                        descriptor.BindStorageBuffer(2, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                        descriptor.BindDescriptor(device, cmdBuffer, &csm.m_pipelines[i]);
                        scene->DrawOpaqueIndexed(cmdBuffer);
                    }
                    if (!scene->maskVertices.empty())
                    {
                        csm.m_maskPipelines[i].BindPipeline(cmdBuffer);

                        ElecNekoDescriptor descriptor = csm.m_maskDescriptors.GetFreeDescriptor();
                        descriptor.BindUniformBuffer(0, &csm.m_viewMatrixBuffers, i * sizeof(Mat4), sizeof(Mat4));
                        descriptor.BindUniformBuffer(1, &csm.m_projMatrixBuffers, i * sizeof(Mat4), sizeof(Mat4));
                        descriptor.BindStorageBuffer(2, &scene->modelMatrixBuffer, 0, scene->modelMatrixBuffer.m_vkBufferSize);
                        descriptor.BindStorageBuffer(3, &scene->materialBuffer, 0, scene->materialBuffer.m_vkBufferSize);
                        descriptor.BindImage(4, VK_IMAGE_LAYOUT_READ_ONLY_OPTIMAL, scene->textureArray->m_arrayImage.m_vkImageView,
                                             ElecNekoSampler::m_samplerCubemap);
                        descriptor.BindDescriptor(device, cmdBuffer, &csm.m_maskPipelines[i]);
                        scene->DrawMaskIndexed(cmdBuffer);
                    }

                    csm.m_shadowMaps[i].EndRenderPass(device, cmdBufferIndex);
                    csm.m_shadowMaps[i].m_imageDepth.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL);
                }
            }

            {
                c_ambientOcclusionRawImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
                c_ambientOcclusionRawPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_ambientOcclusionRawPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, camOffset, camSize);
                descriptor.BindingStorageImage(1, VK_IMAGE_LAYOUT_GENERAL, c_ambientOcclusionRawImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &c_ambientOcclusionRawPipeline);
                GTAOPushConstant_t pushConstant;
                pushConstant.width = c_ambientOcclusionRawImage.m_parms.width;
                pushConstant.height = c_ambientOcclusionRawImage.m_parms.height;
                pushConstant.radius = 1.5f;
                pushConstant.sampleCount = 16;
                pushConstant.bias = 0.02f;
                pushConstant.intensity = 1.0f;
                pushConstant.maxDistance = 1.5f;
                c_ambientOcclusionRawPipeline.PushConstants(cmdBuffer, &pushConstant, sizeof(GTAOPushConstant_t));
                int groupX = (c_ambientOcclusionRawImage.m_parms.width + 15) / 16;
                int groupY = (c_ambientOcclusionRawImage.m_parms.height + 15) / 16;
                c_ambientOcclusionRawPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
                c_ambientOcclusionRawImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            {
                c_ambientOcclusionBlurImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
                c_ambientOcclusionBlurHPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_ambientOcclusionBlurHPipeline.GetFreeDescriptorCompute();
                descriptor.BindingStorageImage(0, VK_IMAGE_LAYOUT_GENERAL, c_ambientOcclusionBlurImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_ambientOcclusionRawImage.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &c_ambientOcclusionBlurHPipeline);
                struct PushConstant_t
                {
                    uint32_t width = c_ambientOcclusionBlurImage.m_parms.width;
                    uint32_t height = c_ambientOcclusionBlurImage.m_parms.height;
                    int radius = 4;
                    float sigma = 4.0f;
                    float depthSigma = 0.05f;
                    float normalSigma = 0.2f;
                } pushConstant;
                c_ambientOcclusionBlurHPipeline.PushConstants(cmdBuffer, &pushConstant, sizeof(PushConstant_t));
                int groupX = (c_ambientOcclusionBlurImage.m_parms.width + 15) / 16;
                int groupY = (c_ambientOcclusionBlurImage.m_parms.height + 15) / 16;
                c_ambientOcclusionBlurHPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
                c_ambientOcclusionBlurImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            {
                c_ambientOcclusionRawImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
                c_ambientOcclusionBlurVPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_ambientOcclusionBlurVPipeline.GetFreeDescriptorCompute();
                descriptor.BindingStorageImage(0, VK_IMAGE_LAYOUT_GENERAL, c_ambientOcclusionRawImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(1, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_ambientOcclusionBlurImage.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(3, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &c_ambientOcclusionBlurVPipeline);
                struct PushConstant_t
                {
                    uint32_t width = c_ambientOcclusionBlurImage.m_parms.width;
                    uint32_t height = c_ambientOcclusionBlurImage.m_parms.height;
                    int radius = 4;
                    float sigma = 4.0f;
                    float depthSigma = 0.05f;
                    float normalSigma = 0.2f;
                } pushConstant;
                c_ambientOcclusionBlurVPipeline.PushConstants(cmdBuffer, &pushConstant, sizeof(PushConstant_t));
                int groupX = (c_ambientOcclusionBlurImage.m_parms.width + 15) / 16;
                int groupY = (c_ambientOcclusionBlurImage.m_parms.height + 15) / 16;
                c_ambientOcclusionBlurVPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
                c_ambientOcclusionRawImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            c_lightingImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

            {
                c_directLightPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_directLightPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, camOffset, camSize);
                descriptor.BindingUniform(1, &csm.m_viewMatrixBuffers, 0, csm.m_viewMatrixBuffers.m_vkBufferSize);
                descriptor.BindingUniform(2, &csm.m_projMatrixBuffers, 0, csm.m_projMatrixBuffers.m_vkBufferSize);
                descriptor.BindingUniform(3, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindingUniform(4, &csm.m_uniformBuffer, 0, csm.m_uniformBuffer.m_vkBufferSize);
                descriptor.BindingStorageImage(5, VK_IMAGE_LAYOUT_GENERAL, c_lightingImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingStorageImage(6, VK_IMAGE_LAYOUT_GENERAL, c_specularLightingImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(7, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageAlbedo.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(8, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(9, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageMaterial.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(10, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageLinearDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(11, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                // descriptor.BindingSampledImage(8, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_shadowFrameBufferEN.m_imageDepth.m_vkImageView,
                //                                ElecNekoSampler::m_samplerShadow);
                descriptor.BindingSampledImage(12, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_environmentCubemap.m_vkImageView, ElecNekoSampler::m_samplerIBL);
                descriptor.BindingSampledImage(13, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_ambientOcclusionRawImage.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                for (int i = 14; i < 14 + csm.numCascade; ++i)
                {
                    descriptor.BindingSampledImage(i, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, csm.m_shadowMaps[i - 14].m_imageDepth.m_vkImageView,
                                                   ElecNekoSampler::m_samplerShadow);
                }
                descriptor.BindDescriptor(device, cmdBuffer, &c_directLightPipeline);
                struct PushConstant_t
                {
                    int useAO;
                    int useContactShadow;
                } push_constant;
                push_constant.useAO = renderOption.useAO;
                push_constant.useContactShadow = renderOption.useCT;
                c_directLightPipeline.PushConstants(cmdBuffer, &push_constant, sizeof(PushConstant_t));
                int groupX = (c_lightingImage.m_parms.width + 15) / 16;
                int groupY = (c_lightingImage.m_parms.height + 15) / 16;
                c_directLightPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);
            }

            c_lightingImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            {
                c_ssrImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

                c_ssrPipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_ssrPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, camOffset, camSize);
                descriptor.BindingStorageImage(1, VK_IMAGE_LAYOUT_GENERAL, c_ssrImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_lightingImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_environmentCubemap.m_vkImageView, ElecNekoSampler::m_samplerIBL);
                descriptor.BindingSampledImage(4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageAlbedo.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageMaterial.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(7, VK_IMAGE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageLinearDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindDescriptor(device, cmdBuffer, &c_ssrPipeline);
                struct PushConstant_t
                {
                    uint32_t width;
                    uint32_t height;

                    int32_t maxSteps;
                    float maxDistance;
                    float strideScale;
                    float thickness;

                    int binaryIters;

                    float roughnessThreshold;
                    float prefilterMaxMip;
                } pushConstants;
                pushConstants.width = c_ssrImage.m_parms.width;
                pushConstants.height = c_ssrImage.m_parms.height;

                pushConstants.maxSteps = renderOption.maxSSRSteps;
                pushConstants.maxDistance = renderOption.maxSSRDistance;
                pushConstants.strideScale = renderOption.strideSSRScale;
                pushConstants.thickness = renderOption.strideSSRScale;

                pushConstants.binaryIters = renderOption.binarySearchSSRIters;
                pushConstants.roughnessThreshold = renderOption.roughnessSSREnabled;
                pushConstants.prefilterMaxMip = static_cast<float>(c_environmentCubemap.m_mipLevels - 1);
                c_ssrPipeline.PushConstants(cmdBuffer, &pushConstants, sizeof(PushConstant_t));
                int groupX = (c_ssrImage.m_parms.width + 15) / 16;
                int groupY = (c_ssrImage.m_parms.height + 15) / 16;
                c_ssrPipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);

                c_ssrImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            {
                c_compositeImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);

                c_compositePipeline.BindPipelineCompute(cmdBuffer);
                ElecNekoDescriptorCompute descriptor = c_compositePipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, camOffset, camSize);
                descriptor.BindingStorageImage(1, VK_IMAGE_LAYOUT_GENERAL, c_compositeImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_lightingImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(3, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_specularLightingImage.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(4, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_ssrImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(5, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageMaterial.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(6, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageLinearDepth.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(7, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, g_geometryFrameBuffer.m_imageNormal.m_vkImageView,
                                               ElecNekoSampler::m_samplerTexture);
                descriptor.BindingSampledImage(8, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_environmentCubemap.m_vkImageView, ElecNekoSampler::m_samplerIBL);
                descriptor.BindDescriptor(device, cmdBuffer, &c_compositePipeline);
                struct pushConst_t
                {
                    int32_t width = c_compositeImage.m_parms.width;
                    int32_t height = c_compositeImage.m_parms.height;
                    float ssrIntensity;
                    float envIntensity;
                } push_const;
                push_const.ssrIntensity = renderOption.ssrStrength;
                push_const.envIntensity = renderOption.envIntensity;
                c_compositePipeline.PushConstants(cmdBuffer, &push_const, sizeof(pushConst_t));
                int groupX = (c_compositeImage.m_parms.width + 15) / 16;
                int groupY = (c_compositeImage.m_parms.height + 15) / 16;
                c_compositePipeline.DispatchCompute(cmdBuffer, groupX, groupY, 1);

                c_compositeImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
            }

            c_postProcessImage.TransitionLayoutEN(cmdBuffer, VK_IMAGE_LAYOUT_GENERAL);
            {
                c_tonemapPipeline.BindPipelineCompute(cmdBuffer);

                ElecNekoDescriptorCompute descriptor = c_tonemapPipeline.GetFreeDescriptorCompute();
                descriptor.BindingUniform(0, uniforms, lightParmsOffset, lightParmsSize);
                descriptor.BindingStorageImage(1, VK_IMAGE_LAYOUT_GENERAL, c_postProcessImage.m_vkImageView, VK_NULL_HANDLE);
                descriptor.BindingSampledImage(2, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, c_compositeImage.m_vkImageView, ElecNekoSampler::m_samplerTexture);
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
