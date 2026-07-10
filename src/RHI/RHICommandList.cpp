#include "RHI/RHICommandList.h"

#include "RHI2/Vulkan/VulkanBuffer.h"

#include <cassert>
#include <vector>

void RHICommandList::SetVertexBuffer(uint32_t slot, RHI::Buffer *buffer, VkDeviceSize offset)
{
    if (buffer == nullptr)
    {
        return;
    }

    RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(buffer);

    assert(vulkanBuffer != nullptr);

    VkBuffer vkBuffer = vulkanBuffer->GetVkBuffer();
    VkDeviceSize vkOffset = offset;

    vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vkBuffer, &vkOffset);
}

void RHICommandList::SetVertexBuffers(uint32_t firstSlot, uint32_t bufferCount, RHI::Buffer **buffers, const VkDeviceSize *offsets)
{
    if (buffers == nullptr || bufferCount == 0)
    {
        return;
    }

    std::vector<VkBuffer> vkBuffers(bufferCount);
    std::vector<VkDeviceSize> vkOffsets(bufferCount);

    for (uint32_t i = 0; i < bufferCount; ++i)
    {
        RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(buffers[i]);

        assert(vulkanBuffer != nullptr);

        vkBuffers[i] = vulkanBuffer->GetVkBuffer();
        vkOffsets[i] = offsets != nullptr ? offsets[i] : 0;
    }

    vkCmdBindVertexBuffers(m_commandBuffer, firstSlot, bufferCount, vkBuffers.data(), vkOffsets.data());
}

void RHICommandList::SetIndexBuffer(RHI::Buffer *buffer, RHIIndexFormat format, VkDeviceSize offset)
{
    if (buffer == nullptr)
    {
        return;
    }

    RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(buffer);

    assert(vulkanBuffer != nullptr);

    VkIndexType vkIndexType = format == RHIIndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

    vkCmdBindIndexBuffer(m_commandBuffer, vulkanBuffer->GetVkBuffer(), offset, vkIndexType);
}
