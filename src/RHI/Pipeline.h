//
//  Pipeline.h
//
#pragma once
#include <vulkan/vulkan.hpp>
#include "Buffer.h"
#include "Descriptor.h"

#include "RHI2/RHIBinding.h"

#include <memory>

class DeviceContext;
class FrameBuffer;
class Descriptors;
class Shader;

namespace ElecNeko
{
    class GFrameBuffer;
    class ElecNekoShader;
} // namespace ElecNeko

/*
====================================================
Pipeline

Think of this as all of the state that's used to draw
====================================================
*/
class Pipeline
{
public:
    Pipeline() {}
    ~Pipeline() {}

    enum CullMode_t
    {
        CULL_MODE_FRONT,
        CULL_MODE_BACK,
        CULL_MODE_NONE
    };

    struct ElecNekoCreateParms_t
    {
        ElecNekoCreateParms_t() { memset(this, 0, sizeof(ElecNekoCreateParms_t)); }
        VkRenderPass renderPass;
        ElecNeko::GFrameBuffer *framebuffer;
        Descriptors *descriptors;
        Shader *shader;

        int width;
        int height;

        CullMode_t cullMode;

        bool depthTest;
        bool depthWrite;

        int pushConstantSize;
        VkShaderStageFlagBits pushConstantShaderStages;
    };

    struct CreateParms_t
    {
        CreateParms_t() { memset(this, 0, sizeof(CreateParms_t)); }
        VkRenderPass renderPass;
        FrameBuffer *framebuffer;
        Descriptors *descriptors;
        ElecNeko::ElecNekoShader *shader;

        int width;
        int height;

        CullMode_t cullMode;

        bool depthTest;
        bool depthWrite;

        int pushConstantSize;
        VkShaderStageFlagBits pushConstantShaderStages;
    };
    bool Create(DeviceContext *device, const CreateParms_t &parms);
    bool CreateForMesh(DeviceContext *device, const CreateParms_t &parms);
    bool CreateForFullScreen(DeviceContext *device, const CreateParms_t &parms);
    bool CreateForTransparency(DeviceContext *device, const CreateParms_t &parms);
    bool CreateCompute(DeviceContext *device, const CreateParms_t &parms);
    void Cleanup(DeviceContext *device);

    Descriptor GetFreeDescriptor() { return m_parms.descriptors->GetFreeDescriptor(); }

    void BindPipeline(VkCommandBuffer cmdBuffer);
    void BindPipelineCompute(VkCommandBuffer cmdBuffer);
    void DispatchCompute(VkCommandBuffer cmdBuffer, int groupCountX, int groupCountY, int groupCountZ);

    CreateParms_t m_parms;

    //
    //	PipelineState
    //
    VkPipelineLayout m_vkPipelineLayout;
    VkPipeline m_vkPipeline;
};


namespace ElecNeko
{
    class ElecNekoPipeline
    {
    public:
        ElecNekoPipeline() = default;
        ~ElecNekoPipeline() = default;

        enum CullMode_t
        {
            CULL_MODE_FRONT,
            CULL_MODE_BACK,
            CULL_MODE_NONE
        };

        enum Usage_t
        {
            USAGE_DEFAULT,
            USAGE_MESH,
            USAGE_STATIC_MESH,
            USAGE_FULL_SCREEN,
            USAGE_TRANSPARENCY
        };

        struct CreateParms_t
        {
            CreateParms_t()
            {
                memset(this, 0, sizeof(CreateParms_t));
                colorAttachmentCount = 1;
                framebuffer = nullptr;
                gFramebuffer = nullptr;
                descriptors = nullptr;
                descriptorsCompute = nullptr;
            }

            VkRenderPass renderPass;
            FrameBuffer *framebuffer;
            GFrameBuffer *gFramebuffer;
            // for graphics pipeline
            ElecNekoDescriptors *descriptors;
            // for compute pipeline
            ElecNekoDescriptorsCompute *descriptorsCompute;
            ElecNekoShader *shader;

            int width;
            int height;

            CullMode_t cullMode;

            bool depthTest;
            bool depthWrite;

            int pushConstantSize;
            VkShaderStageFlagBits pushConstantShaderStages;

            int colorAttachmentCount;
        };
        bool Create(DeviceContext *device, const CreateParms_t &parms, Usage_t usage = USAGE_DEFAULT);
        bool CreateCompute(DeviceContext *device, const CreateParms_t &parms);
        void Cleanup(DeviceContext *device);

        ElecNekoDescriptor GetFreeDescriptor() { return m_parms.descriptors->GetFreeDescriptor(); }
        ElecNekoDescriptorCompute GetFreeDescriptorCompute() { return m_parms.descriptorsCompute->GetFreeDescriptor(); }

        void BindBindingSetDesc(DeviceContext *device, VkCommandBuffer vkCommandBuffer, const RHI::BindingSetDesc &desc);

        void PushConstants(VkCommandBuffer cmdBuffer, const void *data, uint32_t size, uint32_t offset = 0);

        void BindPipeline(VkCommandBuffer cmdBuffer);
        void BindPipelineCompute(VkCommandBuffer cmdBuffer);
        void DispatchCompute(VkCommandBuffer cmdBuffer, int groupCountX, int groupCountY, int groupCountZ);

        void DrawFullScreen(VkCommandBuffer cmdBuffer, uint32_t vertexCount, uint32_t indexCount, uint32_t firstVertex = 0, uint32_t firstIndex = 0);

        CreateParms_t m_parms;

        //
        //	PipelineState
        //
        VkPipelineLayout m_vkPipelineLayout = VK_NULL_HANDLE;
        VkPipeline m_vkPipeline = VK_NULL_HANDLE;
    };
} // namespace ElecNeko
