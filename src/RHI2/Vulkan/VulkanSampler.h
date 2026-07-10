#pragma once

#include "RHI2/RHISampler.h"

#include <vulkan/vulkan.h>

namespace RHI
{
    class VulkanContext;

    class VulkanSampler final : public Sampler
    {
    public:
        VulkanSampler(VulkanContext *context, const SamplerDesc &desc);
        ~VulkanSampler() override;

        bool Create();
        void Destroy();

        const SamplerDesc &GetDesc() const override;

        VkSampler GetVkSampler() const;

    private:
        VkFilter TranslateFilter(Filter filter) const;
        VkSamplerMipmapMode TranslateMipmapMode(SamplerMipmapMode mode) const;
        VkSamplerAddressMode TranslateAddressMode(AddressMode mode) const;
        VkCompareOp TranslateCompareOp(CompareOp op) const;
        VkBorderColor TranslateBorderColor(BorderColor color) const;

    private:
        VulkanContext *m_context = nullptr;
        SamplerDesc m_desc{};

        VkSampler m_sampler = VK_NULL_HANDLE;
    };
} // namespace RHI
