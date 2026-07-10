#include "RHI2/Vulkan/VulkanTexture.h"
#include "RHI2/Vulkan/VulkanContext.h"
#include "RHI2/Vulkan/VulkanUpload.h"


#include <cassert>

namespace RHI
{
    VulkanTexture::VulkanTexture(VulkanContext *context, const TextureDesc &desc) : m_context(context), m_desc(desc) {}

    VulkanTexture::~VulkanTexture() { Destroy(); }

    VkFormat VulkanTexture::TranslateFormat(Format format) const
    {
        switch (format)
        {
            case Format::RGBA8_UNorm:
                return VK_FORMAT_R8G8B8A8_UNORM;
            case Format::RGBA8_SRGB:
                return VK_FORMAT_R8G8B8A8_SRGB;
            case Format::RGBA16_Float:
                return VK_FORMAT_R16G16B16A16_SFLOAT;
            case Format::RGBA32_Float:
                return VK_FORMAT_R32G32B32A32_SFLOAT;
            case Format::D32_Float:
                return VK_FORMAT_D32_SFLOAT;
            default:
                return VK_FORMAT_UNDEFINED;
        }
    }

    VkImageUsageFlags VulkanTexture::TranslateUsage(TextureUsage usage) const
    {
        VkImageUsageFlags flags = 0;

        if (HasFlag(usage, TextureUsage::Sampled))
        {
            flags |= VK_IMAGE_USAGE_SAMPLED_BIT;
        }

        if (HasFlag(usage, TextureUsage::RenderTarget))
        {
            flags |= VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        }

        if (HasFlag(usage, TextureUsage::DepthStencil))
        {
            flags |= VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT;
        }

        if (HasFlag(usage, TextureUsage::TransferSrc))
        {
            flags |= VK_IMAGE_USAGE_TRANSFER_SRC_BIT;
        }

        if (HasFlag(usage, TextureUsage::TransferDst))
        {
            flags |= VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        }

        if (HasFlag(usage, TextureUsage::Storage))
        {
            flags |= VK_IMAGE_USAGE_STORAGE_BIT;
        }

        return flags;
    }

    bool VulkanTexture::Create2DArray()
    {
        assert(m_context != nullptr);

        Destroy();

        m_vkFormat = TranslateFormat(m_desc.format);

        if (m_vkFormat == VK_FORMAT_UNDEFINED)
        {
            return false;
        }

        if (!CreateImage())
        {
            return false;
        }

        if (!CreateImageView())
        {
            Destroy();
            return false;
        }

        return true;
    }

    bool VulkanTexture::CreateImage()
    {
        VkImageCreateInfo imageInfo{};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.pNext = nullptr;
        imageInfo.flags = 0;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = m_vkFormat;
        imageInfo.extent.width = m_desc.width;
        imageInfo.extent.height = m_desc.height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = m_desc.mipLevels;
        imageInfo.arrayLayers = m_desc.layers;
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageInfo.usage = TranslateUsage(m_desc.usage);
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        VkResult result = vkCreateImage(m_context->GetVkDevice(), &imageInfo, nullptr, &m_image);

        if (result != VK_SUCCESS)
        {
            m_image = VK_NULL_HANDLE;
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetImageMemoryRequirements(m_context->GetVkDevice(), m_image, &memoryRequirements);

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = m_context->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        result = vkAllocateMemory(m_context->GetVkDevice(), &allocateInfo, nullptr, &m_memory);

        if (result != VK_SUCCESS)
        {
            vkDestroyImage(m_context->GetVkDevice(), m_image, nullptr);
            m_image = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            return false;
        }

        result = vkBindImageMemory(m_context->GetVkDevice(), m_image, m_memory, 0);

        if (result != VK_SUCCESS)
        {
            Destroy();
            return false;
        }

        m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (m_desc.debugName != nullptr)
        {
            m_context->SetObjectName(reinterpret_cast<uint64_t>(m_image), VK_OBJECT_TYPE_IMAGE, m_desc.debugName);
        }

        return true;
    }

    bool VulkanTexture::CreateImageView()
    {
        VkImageAspectFlags aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

        if (m_desc.format == Format::D32_Float)
        {
            aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }

        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.pNext = nullptr;
        viewInfo.flags = 0;
        viewInfo.image = m_image;
        viewInfo.viewType = (m_desc.force2DArrayView || m_desc.layers > 1) ? VK_IMAGE_VIEW_TYPE_2D_ARRAY : VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = m_vkFormat;
        viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        viewInfo.subresourceRange.aspectMask = aspectMask;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = m_desc.mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = m_desc.layers;

        VkResult result = vkCreateImageView(m_context->GetVkDevice(), &viewInfo, nullptr, &m_imageView);

        if (result != VK_SUCCESS)
        {
            return false;
        }

        if (m_desc.debugName != nullptr)
        {
            m_context->SetObjectName(reinterpret_cast<uint64_t>(m_imageView), VK_OBJECT_TYPE_IMAGE_VIEW, m_desc.debugName);
        }

        return true;

        return result == VK_SUCCESS;
    }

    void VulkanTexture::TransitionImageLayout(VkCommandBuffer commandBuffer, VkImageLayout oldLayout, VkImageLayout newLayout)
    {
        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.pNext = nullptr;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_image;

        if (m_desc.format == Format::D32_Float)
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        }
        else
        {
            barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        }

        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = m_desc.mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = m_desc.layers;

        VkPipelineStageFlags srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }
        else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
            srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        }

        vkCmdPipelineBarrier(commandBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_currentLayout = newLayout;
    }

    void VulkanTexture::Destroy()
    {
        if (m_context == nullptr)
        {
            m_image = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_imageView = VK_NULL_HANDLE;
            m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
            return;
        }

        if (m_imageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(m_context->GetVkDevice(), m_imageView, nullptr);

            m_imageView = VK_NULL_HANDLE;
        }

        if (m_image != VK_NULL_HANDLE)
        {
            vkDestroyImage(m_context->GetVkDevice(), m_image, nullptr);

            m_image = VK_NULL_HANDLE;
        }

        if (m_memory != VK_NULL_HANDLE)
        {
            vkFreeMemory(m_context->GetVkDevice(), m_memory, nullptr);

            m_memory = VK_NULL_HANDLE;
        }

        m_currentLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    const TextureDesc &VulkanTexture::GetDesc() const { return m_desc; }

    VkImageView VulkanTexture::GetVkImageView() const { return m_imageView; }

    VkImage VulkanTexture::GetVkImage() const { return m_image; }

    VkDeviceMemory VulkanTexture::GetVkMemory() const { return m_memory; }

    VkImageLayout VulkanTexture::GetCurrentLayout() const { return m_currentLayout; }

    void VulkanTexture::SetCurrentLayout(VkImageLayout layout) { m_currentLayout = layout; }
} // namespace RHI
