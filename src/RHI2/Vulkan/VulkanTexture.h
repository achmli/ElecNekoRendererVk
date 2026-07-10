#pragma once

#include "RHI2/RHITexture.h"

#include <vulkan/vulkan.h>

namespace RHI
{
    class VulkanContext;

    class VulkanTexture final : public Texture
    {
    public:
        VulkanTexture(VulkanContext *context, const TextureDesc &desc);
        ~VulkanTexture() override;

        bool Create2DArray();

        void Destroy();

        const TextureDesc &GetDesc() const override;
        VkImageView GetVkImageView() const;

        VkImage GetVkImage() const;
        VkDeviceMemory GetVkMemory() const;

        void TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout);

        VkImageLayout GetCurrentLayout() const;
        void SetCurrentLayout(VkImageLayout layout);

    private:
        VkFormat TranslateFormat(Format format) const;
        VkImageUsageFlags TranslateUsage(TextureUsage usage) const;

        bool CreateImage();
        bool CreateImageView();

    private:
        VulkanContext *m_context = nullptr;
        TextureDesc m_desc{};

        VkImage m_image = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkImageView m_imageView = VK_NULL_HANDLE;
        VkFormat m_vkFormat = VK_FORMAT_UNDEFINED;
        VkImageLayout m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    };
} // namespace RHI
