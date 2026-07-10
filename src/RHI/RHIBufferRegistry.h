#pragma once

#include "RHI2/RHIBuffer.h"
#include "RHIHandle.h"


#include <vector>

class Buffer;

namespace RHI
{
    class BufferRegistry
    {
    public:
        BufferHandle Register(::Buffer *buffer);
        BufferHandle Register(RHI::Buffer *buffer);

        ::Buffer *GetLegacy(BufferHandle handle);
        const ::Buffer *GetLegacy(BufferHandle handle) const;

        RHI::Buffer *GetRHI(BufferHandle handle);
        const RHI::Buffer *GetRHI(BufferHandle handle) const;

        void Clear();

    private:
        struct BufferEntry
        {
            ::Buffer *legacyBuffer = nullptr;
            RHI::Buffer *rhiBuffer = nullptr;
        };

        std::vector<BufferEntry> m_buffers;
    };
} // namespace RHI
