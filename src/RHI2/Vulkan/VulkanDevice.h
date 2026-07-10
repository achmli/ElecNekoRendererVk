#pragma once

#include "RHI2/RHIDevice.h"
#include "RHI2/RHIUpload.h"

class DeviceContext;

namespace RHI
{
    class VulkanUploadBatch;
    class Sampler;
    class VulkanContext;

    class VulkanDevice final : public Device
    {
    public:
        explicit VulkanDevice(DeviceContext *device);
        ~VulkanDevice() override;

        Backend GetBackend() const override;

        std::unique_ptr<Buffer> CreateBuffer(const BufferDesc &desc, const void *initialData = nullptr, UploadBatch *uploadBatch = nullptr) override;

        std::unique_ptr<Texture> CreateTexture(const TextureDesc &desc, const void *initialData = nullptr, uint64_t initialDataSize = 0,
                                               UploadBatch *uploadBatch = nullptr) override;

        std::unique_ptr<Sampler> CreateSampler(const SamplerDesc &desc) override;

        std::unique_ptr<UploadBatch> CreateUploadBatch() override;

        void WaitIdle() override;

        void FlushDeferredDeletes() override;

    private:
        std::unique_ptr<VulkanContext> m_context;
    };
} // namespace RHI
