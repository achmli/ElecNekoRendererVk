#include "RHI/RHIBufferRegistry.h"

#include "RHI/Buffer.h"
#include "RHI2/Vulkan/VulkanBuffer.h"

#include <cassert>

namespace RHI
{
    BufferHandle BufferRegistry::Register(::Buffer *buffer)
    {
        BufferHandle handle;
        handle.index = static_cast<uint32_t>(m_buffers.size());

        BufferEntry entry{};
        entry.legacyBuffer = buffer;
        entry.rhiBuffer = nullptr;

        m_buffers.push_back(entry);
        return handle;
    }

    BufferHandle BufferRegistry::Register(RHI::Buffer *buffer)
    {
        BufferHandle handle;
        handle.index = static_cast<uint32_t>(m_buffers.size());

        BufferEntry entry{};
        entry.legacyBuffer = nullptr;
        entry.rhiBuffer = buffer;

        m_buffers.push_back(entry);
        return handle;
    }

    ::Buffer *BufferRegistry::GetLegacy(BufferHandle handle)
    {
        assert(handle.IsValid());
        assert(handle.index < m_buffers.size());

        BufferEntry &entry = m_buffers[handle.index];

        if (entry.legacyBuffer != nullptr)
        {
            return entry.legacyBuffer;
        }

        if (entry.rhiBuffer != nullptr)
        {
            RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(entry.rhiBuffer);

            assert(vulkanBuffer != nullptr);

            return vulkanBuffer->GetLegacyBufferForTransition();
        }

        return nullptr;
    }

    const ::Buffer *BufferRegistry::GetLegacy(BufferHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_buffers.size());

        const BufferEntry &entry = m_buffers[handle.index];

        if (entry.legacyBuffer != nullptr)
        {
            return entry.legacyBuffer;
        }

        if (entry.rhiBuffer != nullptr)
        {
            const RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<const RHI::VulkanBuffer *>(entry.rhiBuffer);

            assert(vulkanBuffer != nullptr);

            return vulkanBuffer->GetLegacyBufferForTransition();
        }

        return nullptr;
    }

    RHI::Buffer *BufferRegistry::GetRHI(BufferHandle handle)
    {
        assert(handle.IsValid());
        assert(handle.index < m_buffers.size());

        return m_buffers[handle.index].rhiBuffer;
    }

    const RHI::Buffer *BufferRegistry::GetRHI(BufferHandle handle) const
    {
        assert(handle.IsValid());
        assert(handle.index < m_buffers.size());

        return m_buffers[handle.index].rhiBuffer;
    }

    void BufferRegistry::Clear() { m_buffers.clear(); }
} // namespace RHI
