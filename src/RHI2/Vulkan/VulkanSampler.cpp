#include "RHI2/Vulkan/VulkanSampler.h"
#include "RHI2/Vulkan/VulkanContext.h"

namespace RHI
{
    VulkanSampler::VulkanSampler(VulkanContext *context, const SamplerDesc &desc) : m_context(context), m_desc(desc) {}

    VulkanSampler::~VulkanSampler() { Destroy(); }

    VkFilter VulkanSampler::TranslateFilter(Filter filter) const
    {
        switch (filter)
        {
            case Filter::Nearest:
                return VK_FILTER_NEAREST;
            case Filter::Linear:
                return VK_FILTER_LINEAR;
            default:
                return VK_FILTER_LINEAR;
        }
    }

    VkSamplerMipmapMode VulkanSampler::TranslateMipmapMode(SamplerMipmapMode mode) const
    {
        switch (mode)
        {
            case SamplerMipmapMode::Nearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;
            case SamplerMipmapMode::Linear:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            default:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
        }
    }

    VkSamplerAddressMode VulkanSampler::TranslateAddressMode(AddressMode mode) const
    {
        switch (mode)
        {
            case AddressMode::Repeat:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
            case AddressMode::ClampToEdge:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE;
            case AddressMode::ClampToBorder:
                return VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER;
            default:
                return VK_SAMPLER_ADDRESS_MODE_REPEAT;
        }
    }

    VkCompareOp VulkanSampler::TranslateCompareOp(CompareOp op) const
    {
        switch (op)
        {
            case CompareOp::Never:
                return VK_COMPARE_OP_NEVER;
            case CompareOp::Less:
                return VK_COMPARE_OP_LESS;
            case CompareOp::Equal:
                return VK_COMPARE_OP_EQUAL;
            case CompareOp::LessOrEqual:
                return VK_COMPARE_OP_LESS_OR_EQUAL;
            case CompareOp::Greater:
                return VK_COMPARE_OP_GREATER;
            case CompareOp::NotEqual:
                return VK_COMPARE_OP_NOT_EQUAL;
            case CompareOp::GreaterOrEqual:
                return VK_COMPARE_OP_GREATER_OR_EQUAL;
            case CompareOp::Always:
                return VK_COMPARE_OP_ALWAYS;
            default:
                return VK_COMPARE_OP_ALWAYS;
        }
    }

    VkBorderColor VulkanSampler::TranslateBorderColor(BorderColor color) const
    {
        switch (color)
        {
            case BorderColor::FloatTransparentBlack:
                return VK_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK;
            case BorderColor::IntTransparentBlack:
                return VK_BORDER_COLOR_INT_TRANSPARENT_BLACK;
            case BorderColor::FloatOpaqueBlack:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
            case BorderColor::IntOpaqueBlack:
                return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
            case BorderColor::FloatOpaqueWhite:
                return VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE;
            case BorderColor::IntOpaqueWhite:
                return VK_BORDER_COLOR_INT_OPAQUE_WHITE;
            default:
                return VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        }
    }

    bool VulkanSampler::Create()
    {
        if (m_context == nullptr)
        {
            return false;
        }

        Destroy();

        VkSamplerCreateInfo samplerInfo{};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.pNext = nullptr;
        samplerInfo.flags = 0;

        samplerInfo.magFilter = TranslateFilter(m_desc.magFilter);
        samplerInfo.minFilter = TranslateFilter(m_desc.minFilter);
        samplerInfo.mipmapMode = TranslateMipmapMode(m_desc.mipmapMode);

        samplerInfo.addressModeU = TranslateAddressMode(m_desc.addressU);
        samplerInfo.addressModeV = TranslateAddressMode(m_desc.addressV);
        samplerInfo.addressModeW = TranslateAddressMode(m_desc.addressW);

        samplerInfo.mipLodBias = m_desc.mipLodBias;

        samplerInfo.anisotropyEnable = m_desc.anisotropyEnable ? VK_TRUE : VK_FALSE;
        samplerInfo.maxAnisotropy = m_desc.maxAnisotropy;

        samplerInfo.compareEnable = m_desc.compareEnable ? VK_TRUE : VK_FALSE;
        samplerInfo.compareOp = TranslateCompareOp(m_desc.compareOp);

        samplerInfo.minLod = m_desc.minLod;
        samplerInfo.maxLod = m_desc.maxLod;

        samplerInfo.borderColor = TranslateBorderColor(m_desc.borderColor);
        samplerInfo.unnormalizedCoordinates = VK_FALSE;

        VkResult result = vkCreateSampler(m_context->GetVkDevice(), &samplerInfo, nullptr, &m_sampler);

        if (result != VK_SUCCESS)
        {
            m_sampler = VK_NULL_HANDLE;
            return false;
        }

        if (m_desc.debugName != nullptr)
        {
            m_context->SetObjectName(reinterpret_cast<uint64_t>(m_sampler), VK_OBJECT_TYPE_SAMPLER, m_desc.debugName);
        }

        return true;
    }

    void VulkanSampler::Destroy()
    {
        if (m_context == nullptr)
        {
            m_sampler = VK_NULL_HANDLE;
            return;
        }

        VkSampler sampler = m_sampler;
        m_sampler = VK_NULL_HANDLE;

        if (sampler == VK_NULL_HANDLE)
        {
            return;
        }

        VkDevice vkDevice = m_context->GetVkDevice();

        m_context->EnqueueDeferredDelete([vkDevice, sampler]() { vkDestroySampler(vkDevice, sampler, nullptr); });
    }

    const SamplerDesc &VulkanSampler::GetDesc() const { return m_desc; }

    VkSampler VulkanSampler::GetVkSampler() const { return m_sampler; }
} // namespace RHI
