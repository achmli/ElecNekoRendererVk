#pragma once

#include "RHI2/RHIBuffer.h"
#include "RHI2/RHISampler.h"
#include "RHI2/RHITexture.h"
#include "RHI2/RHIUpload.h"


#include <memory>

namespace RHI
{
    class Device
    {
    public:
        virtual ~Device() = default;

        virtual Backend GetBackend() const = 0;

        virtual std::unique_ptr<Buffer> CreateBuffer(const BufferDesc &desc, const void *initialData = nullptr, UploadBatch *uploadBatch = nullptr) = 0;

        virtual std::unique_ptr<Texture> CreateTexture(const TextureDesc &desc, const void *initialData = nullptr, uint64_t initialDataSize = 0,
                                                       UploadBatch *uploadBatch = nullptr) = 0;

        virtual std::unique_ptr<Sampler> CreateSampler(const SamplerDesc &desc) = 0;

        virtual std::unique_ptr<UploadBatch> CreateUploadBatch() = 0;

        virtual void WaitIdle() = 0;

        virtual void FlushDeferredDeletes() = 0;
    };
} // namespace RHI
