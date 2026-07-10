#include "RHI2/Vulkan/VulkanUpload.h"
#include "RHI2/Vulkan/VulkanContext.h"

#include <cstring>

namespace RHI
{
    static bool CreateUploadBuffer(VulkanContext *context, VkDeviceSize size, const char *debugName, VkBuffer &outBuffer, VkDeviceMemory &outMemory)
    {
        outBuffer = VK_NULL_HANDLE;
        outMemory = VK_NULL_HANDLE;

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = size;
        bufferInfo.usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(context->GetVkDevice(), &bufferInfo, nullptr, &outBuffer);

        if (result != VK_SUCCESS)
        {
            outBuffer = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(context->GetVkDevice(), outBuffer, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex =
                context->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

        result = vkAllocateMemory(context->GetVkDevice(), &allocateInfo, nullptr, &outMemory);

        if (result != VK_SUCCESS)
        {
            vkDestroyBuffer(context->GetVkDevice(), outBuffer, nullptr);
            outBuffer = VK_NULL_HANDLE;
            outMemory = VK_NULL_HANDLE;
            return false;
        }

        result = vkBindBufferMemory(context->GetVkDevice(), outBuffer, outMemory, 0);

        if (result != VK_SUCCESS)
        {
            vkDestroyBuffer(context->GetVkDevice(), outBuffer, nullptr);
            vkFreeMemory(context->GetVkDevice(), outMemory, nullptr);
            outBuffer = VK_NULL_HANDLE;
            outMemory = VK_NULL_HANDLE;
            return false;
        }

        if (debugName != nullptr)
        {
            context->SetObjectName(reinterpret_cast<uint64_t>(outBuffer), VK_OBJECT_TYPE_BUFFER, debugName);
        }

        return true;
    }

    static bool FillUploadBuffer(VulkanContext *context, VkDeviceMemory memory, const void *data, uint64_t dataSize)
    {
        void *mapped = nullptr;

        VkResult result = vkMapMemory(context->GetVkDevice(), memory, 0, static_cast<VkDeviceSize>(dataSize), 0, &mapped);

        if (result != VK_SUCCESS || mapped == nullptr)
        {
            return false;
        }

        std::memcpy(mapped, data, static_cast<size_t>(dataSize));
        vkUnmapMemory(context->GetVkDevice(), memory);

        return true;
    }

    static void DestroyUploadBuffer(VulkanContext *context, VkBuffer buffer, VkDeviceMemory memory)
    {
        if (context == nullptr)
        {
            return;
        }

        if (buffer == VK_NULL_HANDLE && memory == VK_NULL_HANDLE)
        {
            return;
        }

        VkDevice vkDevice = context->GetVkDevice();

        context->EnqueueDeferredDelete(
                [vkDevice, buffer, memory]()
                {
                    if (buffer != VK_NULL_HANDLE)
                    {
                        vkDestroyBuffer(vkDevice, buffer, nullptr);
                    }

                    if (memory != VK_NULL_HANDLE)
                    {
                        vkFreeMemory(vkDevice, memory, nullptr);
                    }
                });
    }

    bool VulkanUpload::UploadBufferData(VulkanContext *context, VkBuffer dstBuffer, const void *data, uint64_t dataSize, const char *debugName)
    {
        if (context == nullptr || dstBuffer == VK_NULL_HANDLE || data == nullptr || dataSize == 0)
        {
            return false;
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        const bool created = CreateUploadBuffer(context, static_cast<VkDeviceSize>(dataSize), debugName, stagingBuffer, stagingMemory);

        if (!created)
        {
            return false;
        }

        const bool filled = FillUploadBuffer(context, stagingMemory, data, dataSize);

        if (!filled)
        {
            DestroyUploadBuffer(context, stagingBuffer, stagingMemory);
            return false;
        }

        VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = static_cast<VkDeviceSize>(dataSize);

        vkCmdCopyBuffer(commandBuffer, stagingBuffer, dstBuffer, 1, &copyRegion);

        context->EndSingleTimeCommands(commandBuffer);

        DestroyUploadBuffer(context, stagingBuffer, stagingMemory);

        return true;
    }

    bool VulkanUpload::UploadTexture2DArrayData(VulkanContext *context, VkImage dstImage, VkImageLayout oldLayout, VkImageLayout finalLayout, uint32_t width,
                                                uint32_t height, uint32_t layers, const void *data, uint64_t dataSize, const char *debugName)
    {
        if (context == nullptr || dstImage == VK_NULL_HANDLE || data == nullptr || dataSize == 0 || width == 0 || height == 0 || layers == 0)
        {
            return false;
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        const bool created = CreateUploadBuffer(context, static_cast<VkDeviceSize>(dataSize), debugName, stagingBuffer, stagingMemory);

        if (!created)
        {
            return false;
        }

        const bool filled = FillUploadBuffer(context, stagingMemory, data, dataSize);

        if (!filled)
        {
            DestroyUploadBuffer(context, stagingBuffer, stagingMemory);
            return false;
        }

        VkCommandBuffer commandBuffer = context->BeginSingleTimeCommands();

        VkImageMemoryBarrier toTransferBarrier{};
        toTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransferBarrier.pNext = nullptr;
        toTransferBarrier.srcAccessMask = 0;
        toTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransferBarrier.oldLayout = oldLayout;
        toTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.image = dstImage;
        toTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toTransferBarrier.subresourceRange.baseMipLevel = 0;
        toTransferBarrier.subresourceRange.levelCount = 1;
        toTransferBarrier.subresourceRange.baseArrayLayer = 0;
        toTransferBarrier.subresourceRange.layerCount = layers;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &toTransferBarrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;

        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = layers;

        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(commandBuffer, stagingBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier toFinalBarrier{};
        toFinalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toFinalBarrier.pNext = nullptr;
        toFinalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toFinalBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toFinalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toFinalBarrier.newLayout = finalLayout;
        toFinalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toFinalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toFinalBarrier.image = dstImage;
        toFinalBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toFinalBarrier.subresourceRange.baseMipLevel = 0;
        toFinalBarrier.subresourceRange.levelCount = 1;
        toFinalBarrier.subresourceRange.baseArrayLayer = 0;
        toFinalBarrier.subresourceRange.layerCount = layers;

        vkCmdPipelineBarrier(commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &toFinalBarrier);

        context->EndSingleTimeCommands(commandBuffer);

        DestroyUploadBuffer(context, stagingBuffer, stagingMemory);

        return true;
    }

    VulkanUploadBatch::VulkanUploadBatch(VulkanContext *context) : m_context(context) {}

    VulkanUploadBatch::~VulkanUploadBatch()
    {
        if (!m_submitted)
        {
            DestroyStagingResources();
        }
    }

    bool VulkanUploadBatch::Begin()
    {
        if (m_context == nullptr || m_recording)
        {
            return false;
        }

        m_commandBuffer = m_context->BeginSingleTimeCommands();

        if (m_commandBuffer == VK_NULL_HANDLE)
        {
            return false;
        }

        m_recording = true;
        m_submitted = false;

        return true;
    }

    bool VulkanUploadBatch::UploadBufferData(VkBuffer dstBuffer, const void *data, uint64_t dataSize, const char *debugName)
    {
        if (!m_recording || m_commandBuffer == VK_NULL_HANDLE || dstBuffer == VK_NULL_HANDLE || data == nullptr || dataSize == 0)
        {
            return false;
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        const bool created = CreateUploadBuffer(m_context, static_cast<VkDeviceSize>(dataSize), debugName, stagingBuffer, stagingMemory);

        if (!created)
        {
            return false;
        }

        const bool filled = FillUploadBuffer(m_context, stagingMemory, data, dataSize);

        if (!filled)
        {
            DestroyUploadBuffer(m_context, stagingBuffer, stagingMemory);
            return false;
        }

        StagingResource staging{};
        staging.buffer = stagingBuffer;
        staging.memory = stagingMemory;
        m_stagingResources.push_back(staging);

        VkBufferCopy copyRegion{};
        copyRegion.srcOffset = 0;
        copyRegion.dstOffset = 0;
        copyRegion.size = static_cast<VkDeviceSize>(dataSize);

        vkCmdCopyBuffer(m_commandBuffer, stagingBuffer, dstBuffer, 1, &copyRegion);

        return true;
    }

    bool VulkanUploadBatch::UploadTexture2DArrayData(VkImage dstImage, VkImageLayout oldLayout, VkImageLayout finalLayout, uint32_t width, uint32_t height,
                                                     uint32_t layers, const void *data, uint64_t dataSize, const char *debugName)
    {
        if (!m_recording || m_commandBuffer == VK_NULL_HANDLE || dstImage == VK_NULL_HANDLE || data == nullptr || dataSize == 0 || width == 0 || height == 0 ||
            layers == 0)
        {
            return false;
        }

        VkBuffer stagingBuffer = VK_NULL_HANDLE;
        VkDeviceMemory stagingMemory = VK_NULL_HANDLE;

        const bool created = CreateUploadBuffer(m_context, static_cast<VkDeviceSize>(dataSize), debugName, stagingBuffer, stagingMemory);

        if (!created)
        {
            return false;
        }

        const bool filled = FillUploadBuffer(m_context, stagingMemory, data, dataSize);

        if (!filled)
        {
            DestroyUploadBuffer(m_context, stagingBuffer, stagingMemory);
            return false;
        }

        StagingResource staging{};
        staging.buffer = stagingBuffer;
        staging.memory = stagingMemory;
        m_stagingResources.push_back(staging);

        VkImageMemoryBarrier toTransferBarrier{};
        toTransferBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toTransferBarrier.pNext = nullptr;
        toTransferBarrier.srcAccessMask = 0;
        toTransferBarrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toTransferBarrier.oldLayout = oldLayout;
        toTransferBarrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toTransferBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toTransferBarrier.image = dstImage;
        toTransferBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toTransferBarrier.subresourceRange.baseMipLevel = 0;
        toTransferBarrier.subresourceRange.levelCount = 1;
        toTransferBarrier.subresourceRange.baseArrayLayer = 0;
        toTransferBarrier.subresourceRange.layerCount = layers;

        vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &toTransferBarrier);

        VkBufferImageCopy copyRegion{};
        copyRegion.bufferOffset = 0;
        copyRegion.bufferRowLength = 0;
        copyRegion.bufferImageHeight = 0;
        copyRegion.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        copyRegion.imageSubresource.mipLevel = 0;
        copyRegion.imageSubresource.baseArrayLayer = 0;
        copyRegion.imageSubresource.layerCount = layers;
        copyRegion.imageOffset = {0, 0, 0};
        copyRegion.imageExtent = {width, height, 1};

        vkCmdCopyBufferToImage(m_commandBuffer, stagingBuffer, dstImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &copyRegion);

        VkImageMemoryBarrier toFinalBarrier{};
        toFinalBarrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        toFinalBarrier.pNext = nullptr;
        toFinalBarrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        toFinalBarrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        toFinalBarrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        toFinalBarrier.newLayout = finalLayout;
        toFinalBarrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toFinalBarrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        toFinalBarrier.image = dstImage;
        toFinalBarrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        toFinalBarrier.subresourceRange.baseMipLevel = 0;
        toFinalBarrier.subresourceRange.levelCount = 1;
        toFinalBarrier.subresourceRange.baseArrayLayer = 0;
        toFinalBarrier.subresourceRange.layerCount = layers;

        vkCmdPipelineBarrier(m_commandBuffer, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0, 0, nullptr, 0, nullptr, 1,
                             &toFinalBarrier);

        return true;
    }

    bool VulkanUploadBatch::SubmitAndWait()
    {
        if (!m_recording || m_commandBuffer == VK_NULL_HANDLE || m_context == nullptr)
        {
            return false;
        }

        m_context->EndSingleTimeCommands(m_commandBuffer);

        m_commandBuffer = VK_NULL_HANDLE;
        m_recording = false;
        m_submitted = true;

        DestroyStagingResources();

        return true;
    }

    void VulkanUploadBatch::DestroyStagingResources()
    {
        for (const StagingResource &resource: m_stagingResources)
        {
            DestroyUploadBuffer(m_context, resource.buffer, resource.memory);
        }

        m_stagingResources.clear();
    }
} // namespace RHI
