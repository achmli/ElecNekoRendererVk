#pragma once

#include "RHI2/RHIUpload.h"

#include <cstdint>
#include <vector>
#include <vulkan/vulkan.h>

namespace RHI
{
    class VulkanContext;

    class VulkanUpload
    {
    public:
        static bool UploadBufferData(VulkanContext *context, VkBuffer dstBuffer, const void *data, uint64_t dataSize, const char *debugName = nullptr);

        static bool UploadTexture2DArrayData(VulkanContext *context, VkImage dstImage, VkImageLayout oldLayout, VkImageLayout finalLayout, uint32_t width,
                                             uint32_t height, uint32_t layers, const void *data, uint64_t dataSize, const char *debugName = nullptr);
    };

    class VulkanUploadBatch final : public UploadBatch
    {
    public:
        explicit VulkanUploadBatch(VulkanContext *context);
        ~VulkanUploadBatch();

        bool Begin() override;

        bool UploadBufferData(VkBuffer dstBuffer, const void *data, uint64_t dataSize, const char *debugName = nullptr);

        bool UploadTexture2DArrayData(VkImage dstImage, VkImageLayout oldLayout, VkImageLayout finalLayout, uint32_t width, uint32_t height, uint32_t layers,
                                      const void *data, uint64_t dataSize, const char *debugName = nullptr);

        bool SubmitAndWait() override;

    private:
        struct StagingResource
        {
            VkBuffer buffer = VK_NULL_HANDLE;
            VkDeviceMemory memory = VK_NULL_HANDLE;
        };

        void DestroyStagingResources();

    private:
        VulkanContext *m_context = nullptr;
        VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
        bool m_recording = false;
        bool m_submitted = false;

        std::vector<StagingResource> m_stagingResources;
    };
} // namespace RHI
