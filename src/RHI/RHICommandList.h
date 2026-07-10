// src/RHI/RHICommandList.h
#pragma once

#include "Buffer.h"
#include "RHIBufferRegistry.h"
#include "RHIHandle.h"

#include "RHI2/RHIBuffer.h"

#include <cstdint>
#include <vulkan/vulkan.h>

namespace RHI
{
    class Buffer;
}

enum class RHIIndexFormat
{
    UInt16,
    UInt32
};

class RHICommandList
{
public:
    RHICommandList() = default;

    RHICommandList(VkCommandBuffer commandBuffer, RHI::BufferRegistry *bufferRegistry) : m_commandBuffer(commandBuffer), m_bufferRegistry(bufferRegistry) {}

    void Reset(VkCommandBuffer commandBuffer, RHI::BufferRegistry *bufferRegistry)
    {
        m_commandBuffer = commandBuffer;
        m_bufferRegistry = bufferRegistry;
    }

    VkCommandBuffer GetNativeCommandBuffer() const { return m_commandBuffer; }

    bool IsValid() const { return m_commandBuffer != VK_NULL_HANDLE && m_bufferRegistry != nullptr; }

    void SetVertexBuffer(RHI::BufferHandle bufferHandle, uint32_t slot = 0, VkDeviceSize offset = 0)
    {
        const Buffer *buffer = m_bufferRegistry->GetLegacy(bufferHandle);
        assert(buffer != nullptr);

        VkBuffer vkBuffer = buffer->m_vkBuffer;
        VkDeviceSize vkOffset = buffer->m_offset + offset;

        vkCmdBindVertexBuffers(m_commandBuffer, slot, 1, &vkBuffer, &vkOffset);
    }

    void SetVertexBuffers(uint32_t firstSlot, uint32_t count, const RHI::BufferHandle *bufferHandles, const VkDeviceSize *offsets)
    {
        VkBuffer vkBuffers[16];
        VkDeviceSize vkOffsets[16];

        assert(count <= 16);

        for (uint32_t i = 0; i < count; ++i)
        {
            const Buffer *buffer = m_bufferRegistry->GetLegacy(bufferHandles[i]);
            assert(buffer != nullptr);

            vkBuffers[i] = buffer->m_vkBuffer;
            vkOffsets[i] = buffer->m_offset + offsets[i];
        }

        vkCmdBindVertexBuffers(m_commandBuffer, firstSlot, count, vkBuffers, vkOffsets);
    }

    void SetIndexBuffer(RHI::BufferHandle bufferHandle, RHIIndexFormat format = RHIIndexFormat::UInt32, VkDeviceSize offset = 0)
    {
        const Buffer *buffer = m_bufferRegistry->GetLegacy(bufferHandle);
        assert(buffer != nullptr);

        VkIndexType vkIndexType = format == RHIIndexFormat::UInt16 ? VK_INDEX_TYPE_UINT16 : VK_INDEX_TYPE_UINT32;

        vkCmdBindIndexBuffer(m_commandBuffer, buffer->m_vkBuffer, buffer->m_offset + offset, vkIndexType);
    }

    void SetVertexBuffer(uint32_t slot, RHI::Buffer *buffer, VkDeviceSize offset = 0);

    void SetVertexBuffers(uint32_t firstSlot, uint32_t bufferCount, RHI::Buffer **buffers, const VkDeviceSize *offsets = nullptr);

    void SetIndexBuffer(RHI::Buffer *buffer, RHIIndexFormat format, VkDeviceSize offset = 0);

    void DrawIndexed(uint32_t indexCount, uint32_t instanceCount = 1, uint32_t firstIndex = 0, int32_t vertexOffset = 0, uint32_t firstInstance = 0)
    {
        vkCmdDrawIndexed(m_commandBuffer, indexCount, instanceCount, firstIndex, vertexOffset, firstInstance);
    }

    void DrawIndexedIndirect(RHI::BufferHandle indirectBufferHandle, VkDeviceSize offset, uint32_t drawCount, uint32_t stride)
    {
        const Buffer *indirectBuffer = m_bufferRegistry->GetLegacy(indirectBufferHandle);
        assert(indirectBuffer != nullptr);

        vkCmdDrawIndexedIndirect(m_commandBuffer, indirectBuffer->m_vkBuffer, indirectBuffer->m_offset + offset, drawCount, stride);
    }

    VkCommandBuffer GetVkCommandBuffer() const { return m_commandBuffer; }

private:
    VkCommandBuffer m_commandBuffer = VK_NULL_HANDLE;
    RHI::BufferRegistry *m_bufferRegistry = nullptr;
};
