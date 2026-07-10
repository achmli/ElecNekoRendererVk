#pragma once

#include "RHI2/RHICommon.h"

#include <cstdint>

namespace RHI
{
    struct BufferDesc
    {
        uint64_t size = 0;
        BufferUsage usage = BufferUsage::None;
        bool cpuVisible = false;
        const char *debugName = nullptr;
    };

    class Buffer
    {
    public:
        virtual ~Buffer() = default;

        virtual const BufferDesc &GetDesc() const = 0;
        virtual uint64_t GetSize() const = 0;

        virtual void *Map() = 0;
        virtual void Unmap() = 0;
    };
} // namespace RHI
