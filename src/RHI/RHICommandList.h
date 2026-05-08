#pragma once

#include "Buffer.h"

#include <cstdint>
#include <vulkan/vulkan.h>

enum class RHIIndexFormat
{
    UInt16,
    UInt32
};

class RHICommandList
{
public:
    RHICommandList() = default;

    explicit RHICommandList(VkCommandBuffer commandBuffer) : m_commandBuffer(commandBuffer) {}

    void Reset(VkCommandBuffer commandBuffer) { m_commandBuffer = commandBuffer; }

    VkCommandBuffer GetNativeCommandBuffer() const { return m_commandBuffer; }

    void SetVertexBuffer(const Buffer &buffer, uint32_t slot = 0, VkDeviceSize offset = 0)
    {
        VkBuffer vkBuffer = buffer.m_vkBuffer;
        VkDeviceSize vkOffset = buffer.m_offset + offset;

        vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vkBuffer, &vkOffset);
    }

    void SetVertexBuffers(uint32_t firstSlot, uint32_t count, const VkBuffer *buffers, const VkDeviceSize *offsets)
    {
        vkCmdBindVertexBuffers(m_commandBuffer, firstSlot, count, buffers, offsets);
    }

    void SetIndexBuffer(const Buffer &buffer, RHIIndexFormat format = RHIIndexFormat::UInt32, VkDeviceSize offset = 0)
    {
        VkIndexType vkIndexType = format == RHIIndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

        vkCmdBindIndexBuffer(m_commandBuffer, buffer.m_vkBuffer, buffer.m_offset + offset, vkIndexType);
    }

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0)
    {
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void DrawIndexedIndirect(const Buffer &indirectBuffer, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        vkCmdDrawIndexedIndirect(m_commandBuffer, indirectBuffer.m_vkBuffer, indirectBuffer.m_offset + offset, drawCount, stride);
    }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
};
