// src/RHI/RHIBufferRegistry.h
#pragma once

#include "RHIHandle.h"

#include <cassert>
#include <vector>

class Buffer;

namespace RHI
{
    class BufferRegistry
    {
    public:
        BufferHandle Register(Buffer *buffer)
        {
            BufferHandle handle;
            handle.index = static_cast<uint32_t>(m_buffers.size());
            m_buffers.push_back(buffer);
            return handle;
        }

        Buffer *Get(BufferHandle handle)
        {
            assert(handle.IsValid());
            assert(handle.index < m_buffers.size());
            return m_buffers[handle.index];
        }

        const Buffer *Get(BufferHandle handle) const
        {
            assert(handle.IsValid());
            assert(handle.index < m_buffers.size());
            return m_buffers[handle.index];
        }

    private:
        std::vector<Buffer *> m_buffers;
    };
} // namespace RHI
