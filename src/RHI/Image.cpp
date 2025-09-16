//
//  Image.cpp
//
#include "Image.h"
#include <assert.h>
#include <stdio.h>

namespace ElecNeko
{
    static LayoutBarrierInfo GetStageAccessForLayout(VkImageLayout layout)
    {
        switch (layout)
        {
            case VK_IMAGE_LAYOUT_UNDEFINED:
                return {VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT, 0};
            case VK_IMAGE_LAYOUT_PREINITIALIZED:
                return {VK_PIPELINE_STAGE_HOST_BIT, VK_ACCESS_HOST_WRITE_BIT};
            case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
                return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_WRITE_BIT};
            case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
                return {VK_PIPELINE_STAGE_TRANSFER_BIT, VK_ACCESS_TRANSFER_READ_BIT};
            case VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL:
                return {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT};
            case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
                return {VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT | VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT, VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT};
            case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
                return {VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT | VK_PIPELINE_STAGE_COMPUTE_SHADER_BIT, VK_ACCESS_SHADER_READ_BIT};
            case VK_IMAGE_LAYOUT_GENERAL:
                return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0};
            default:
                return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
        }
    }
} // namespace ElecNeko

/*
========================================================================================================

Image

========================================================================================================
*/

/*
====================================================
Image::Cleanup
====================================================
*/
bool Image::Create(DeviceContext *device, const CreateParms_t &parms)
{
    VkResult result;

    m_parms = parms;

    //
    //	Create the Image
    //

    VkImageCreateInfo image = {};
    image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    image.imageType = VK_IMAGE_TYPE_1D;
    if (m_parms.height > 1)
    {
        image.imageType = VK_IMAGE_TYPE_2D;
    }
    if (m_parms.depth > 1)
    {
        image.imageType = VK_IMAGE_TYPE_3D;
    }

    image.extent.width = m_parms.width;
    image.extent.height = m_parms.height;
    image.extent.depth = m_parms.depth;
    image.mipLevels = 1;
    image.arrayLayers = 1;
    image.samples = VK_SAMPLE_COUNT_1_BIT;
    image.tiling = VK_IMAGE_TILING_OPTIMAL;
    image.format = m_parms.format;
    if (VK_FORMAT_D32_SFLOAT == m_parms.format)
    {
        image.usage = VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    else
    {
        image.usage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
    }
    image.usage = parms.usageFlags;

    result = vkCreateImage(device->m_vkDevice, &image, nullptr, &m_vkImage);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create image\n");
        assert(0);
        return false;
    }

    //
    //	Allocate memory on the GPU and attach it to the
    //

    VkMemoryAllocateInfo memAlloc = {};
    memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    VkMemoryRequirements memReqs;
    vkGetImageMemoryRequirements(device->m_vkDevice, m_vkImage, &memReqs);
    memAlloc.allocationSize = memReqs.size;
    memAlloc.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

    result = vkAllocateMemory(device->m_vkDevice, &memAlloc, nullptr, &m_vkDeviceMemory);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to allocate memory\n");
        assert(0);
        return false;
    }

    result = vkBindImageMemory(device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to bind image memory\n");
        assert(0);
        return false;
    }

    //
    //	Create the image view
    //

    VkImageViewCreateInfo imageView = {};
    imageView.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    imageView.viewType = VK_IMAGE_VIEW_TYPE_1D;
    if (m_parms.height > 1)
    {
        imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    }
    if (m_parms.depth > 1)
    {
        imageView.viewType = VK_IMAGE_VIEW_TYPE_3D;
    }

    imageView.format = m_parms.format;
    imageView.subresourceRange = {};
    if (VK_FORMAT_D32_SFLOAT == m_parms.format)
    {
        imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        imageView.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    imageView.subresourceRange.baseMipLevel = 0;
    imageView.subresourceRange.levelCount = 1;
    imageView.subresourceRange.baseArrayLayer = 0;
    imageView.subresourceRange.layerCount = 1;
    imageView.image = m_vkImage;

    result = vkCreateImageView(device->m_vkDevice, &imageView, nullptr, &m_vkImageView);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create image view\n");
        assert(0);
        return false;
    }

    return true;
}

/*
====================================================
Image::Cleanup
====================================================
*/
void Image::Cleanup(DeviceContext *device)
{
    vkDestroyImageView(device->m_vkDevice, m_vkImageView, nullptr);
    vkDestroyImage(device->m_vkDevice, m_vkImage, nullptr);
    vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
}

/*
====================================================
Image::TransitionLayout
====================================================
*/
void Image::TransitionLayout(DeviceContext *device)
{
    // Transition the image layout
    VkCommandBuffer vkCommandBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_vkImage;
    if (VK_FORMAT_D32_SFLOAT == m_parms.format)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(vkCommandBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    device->FlushCommandBuffer(vkCommandBuffer, device->m_vkGraphicsQueue);

    m_vkImageLayout = VK_IMAGE_LAYOUT_GENERAL;
}

/*
====================================================
Image::TransitionLayout
====================================================
*/
void Image::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout)
{
    if (m_vkImageLayout == newLayout)
    {
        return;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = m_vkImageLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_vkImage;
    if (VK_FORMAT_D32_SFLOAT == m_parms.format)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;
    barrier.srcAccessMask = 0;
    barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

    VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
    VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

    vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_vkImageLayout = newLayout;
}

void Image::TransitionLayoutEN(VkCommandBuffer cmdBuffer, VkImageLayout newLayout)
{
    VkImageLayout oldLayout = m_vkImageLayout;

    if (oldLayout == newLayout)
    {
        return;
    }

    if (oldLayout == (VkImageLayout) 0)
    {
        oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    VkImageMemoryBarrier barrier = {};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrier.image = m_vkImage;

    if (m_parms.format == VK_FORMAT_D32_SFLOAT || m_parms.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_parms.format == VK_FORMAT_D24_UNORM_S8_UINT)
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
        if (m_parms.format == VK_FORMAT_D32_SFLOAT_S8_UINT || m_parms.format == VK_FORMAT_D24_UNORM_S8_UINT)
        {
            barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
        }
    }
    else
    {
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    }

    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    ElecNeko::LayoutBarrierInfo srcInfo = ElecNeko::GetStageAccessForLayout(oldLayout);
    ElecNeko::LayoutBarrierInfo dstInfo = ElecNeko::GetStageAccessForLayout(newLayout);

    barrier.srcAccessMask = srcInfo.access;
    barrier.dstAccessMask = dstInfo.access;

    VkPipelineStageFlags srcStage = srcInfo.stage;
    VkPipelineStageFlags dstStage = dstInfo.stage;

    vkCmdPipelineBarrier(cmdBuffer, srcStage, dstStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

    m_vkImageLayout = newLayout;
}

namespace ElecNeko
{
    bool CubeImage::Create(DeviceContext *device, const int width, const int height)
    {
        // Create Image
        VkImageCreateInfo imageCI{};
        imageCI.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageCI.imageType = VK_IMAGE_TYPE_2D;
        imageCI.format = VK_FORMAT_R8G8B8A8_UNORM;
        imageCI.extent.width = width;
        imageCI.extent.height = height;
        imageCI.extent.depth = 1;
        imageCI.mipLevels = 1;
        imageCI.arrayLayers = 6;
        imageCI.samples = VK_SAMPLE_COUNT_1_BIT;
        imageCI.tiling = VK_IMAGE_TILING_OPTIMAL;
        imageCI.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
        imageCI.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageCI.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device->m_vkDevice, &imageCI, nullptr, &m_vkImage) != VK_SUCCESS)
        {
            printf("ERROR: Failed to Create Cube Image! \n");
            assert(0);
            return false;
        }

        //	Allocate memory on the GPU and attach it to the
        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device->m_vkDevice, m_vkImage, &memReqs);
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device->m_vkDevice, &memAlloc, nullptr, &m_vkDeviceMemory) != VK_SUCCESS)
        {
            printf("ERROR: Failed to allocate memory\n");
            assert(0);
            return false;
        }

        if (vkBindImageMemory(device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0) != VK_SUCCESS)
        {
            printf("ERROR: Failed to bind image memory\n");
            assert(0);
            return false;
        }

        // Create Image View
        VkImageViewCreateInfo viewCI{};
        viewCI.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewCI.image = m_vkImage;
        viewCI.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewCI.format = imageCI.format;
        viewCI.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewCI.subresourceRange.baseMipLevel = 0;
        viewCI.subresourceRange.levelCount = 1;
        viewCI.subresourceRange.baseArrayLayer = 0;
        viewCI.subresourceRange.layerCount = 6;

        if (vkCreateImageView(device->m_vkDevice, &viewCI, nullptr, &m_vkImageView) != VK_SUCCESS)
        {
            printf("ERROR: Failed to Create Cube Image View! \n");
            assert(0);
            return false;
        }

        return true;
    }

    void CubeImage::Cleanup(DeviceContext *device)
    {
        vkDestroyImageView(device->m_vkDevice, m_vkImageView, nullptr);
        vkDestroyImage(device->m_vkDevice, m_vkImage, nullptr);
        vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
    }

    void CubeImage::TransitionLayout(DeviceContext *device)
    {
        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);

        m_vkImageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    void CubeImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout)
    {
        if (m_vkImageLayout == newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_vkImageLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;
        // barrier.srcAccessMask = 0;
        // barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //
        // VkPipelineStageFlags sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        // VkPipelineStageFlags destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        LayoutBarrierInfo srcInfo = GetStageAccessForLayout(m_vkImageLayout);
        LayoutBarrierInfo dstInfo = GetStageAccessForLayout(newLayout);

        barrier.srcAccessMask = srcInfo.access;
        barrier.dstAccessMask = dstInfo.access;

        VkPipelineStageFlags sourceStage = srcInfo.stage;
        VkPipelineStageFlags destinationStage = dstInfo.stage;

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_vkImageLayout = newLayout;
    }

    bool ArrayImage::Create(DeviceContext *device, const CreateParms_t &parms)
    {
        m_parms.width = parms.width;
        m_parms.height = parms.height;
        m_parms.arrayLayers = parms.arrayLayers;
        m_parms.format = parms.format;
        m_parms.usageFlags = parms.usageFlags;

        VkImageCreateInfo image = {};
        image.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        image.imageType = VK_IMAGE_TYPE_2D;
        image.format = m_parms.format;
        image.extent.width = static_cast<uint32_t>(m_parms.width);
        image.extent.height = static_cast<uint32_t>(m_parms.height);
        image.extent.depth = 1;
        image.mipLevels = 1;
        image.arrayLayers = static_cast<uint32_t>(m_parms.arrayLayers);
        image.samples = VK_SAMPLE_COUNT_1_BIT;
        image.tiling = VK_IMAGE_TILING_OPTIMAL;
        image.usage = m_parms.usageFlags;
        image.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        image.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        image.flags = 0;

        if (vkCreateImage(device->m_vkDevice, &image, nullptr, &m_vkImage) != VK_SUCCESS)
        {
            printf("ERROR: Failed to Create Array Image! \n");
            assert(0);
            return false;
        }

        // allocate memory
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device->m_vkDevice, m_vkImage, &memReqs);

        VkMemoryAllocateInfo memAlloc = {};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device->m_vkDevice, &memAlloc, nullptr, &m_vkDeviceMemory) != VK_SUCCESS)
        {
            printf("ERROR: Failed to allocate memory\n");
            assert(0);
            return false;
        }

        if (vkBindImageMemory(device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0) != VK_SUCCESS)
        {
            printf("ERROR: Failed to bind image memory\n");
            assert(0);
            return false;
        }

        // Create array image view
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_vkImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D_ARRAY;
        viewInfo.format = m_parms.format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = 1;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = static_cast<uint32_t>(m_parms.arrayLayers);
        if (vkCreateImageView(device->m_vkDevice, &viewInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
        {
            printf("ERROR: Failed to Create Array Image View! \n");
            assert(0);
            return false;
        }

        return true;
    }

    void ArrayImage::Cleanup(DeviceContext *device)
    {
        vkDestroyImageView(device->m_vkDevice, m_vkImageView, nullptr);
        vkDestroyImage(device->m_vkDevice, m_vkImage, nullptr);
        vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
    }

    void ArrayImage::TransitionLayout(DeviceContext *device)
    {
        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_GENERAL;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = static_cast<uint32_t>(m_parms.arrayLayers);

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);
        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);
        m_vkImageLayout = VK_IMAGE_LAYOUT_GENERAL;
    }

    void ArrayImage::TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout)
    {
        if (m_vkImageLayout == newLayout)
        {
            return;
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_vkImageLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = static_cast<uint32_t>(m_parms.arrayLayers);


        LayoutBarrierInfo srcInfo = GetStageAccessForLayout(m_vkImageLayout);
        LayoutBarrierInfo dstInfo = GetStageAccessForLayout(newLayout);

        barrier.srcAccessMask = srcInfo.access;
        barrier.dstAccessMask = dstInfo.access;

        VkPipelineStageFlags sourceStage = srcInfo.stage;
        VkPipelineStageFlags destStage = dstInfo.stage;


        // if (m_vkImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_GENERAL)
        // {
        //     barrier.srcAccessMask = 0;
        //     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //
        //     sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        //     destFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        // }
        // else if (m_vkImageLayout == VK_IMAGE_LAYOUT_GENERAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        // {
        //     barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //     barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        //
        //     sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        //     destFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        // }
        // else if (m_vkImageLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL)
        // {
        //     barrier.srcAccessMask = 0;
        //     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //     sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        //     destFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        // }
        // else if (m_vkImageLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL)
        // {
        //     barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //     barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;
        //     sourceFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        //     destFlags = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        // }
        // else
        // {
        //     barrier.srcAccessMask = 0;
        //     barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        //     sourceFlags = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
        //     destFlags = VK_PIPELINE_STAGE_TRANSFER_BIT;
        // }

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_vkImageLayout = newLayout;
    }

} // namespace ElecNeko
