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
                return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_SHADER_WRITE_BIT};
            case VK_IMAGE_LAYOUT_PRESENT_SRC_KHR:
                return {VK_PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT | VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT, 0};
            default:
                return {VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT};
        }
    }

    static float HalfToFloat(uint16_t h)
    {
        uint32_t sign = (h & 0x8000u) << 16;
        uint32_t exp = (h & 0x7c00u) >> 10;
        uint32_t mant = (h & 0x03ffu);

        uint32_t outBits = 0;

        if (exp == 0u)
        {
            if (mant == 0u)
            {
                outBits = sign;
            }
            else
            {
                int32_t e = -14;
                while ((mant & 0x0400u) == 0u)
                {
                    mant <<= 1;
                    e -= 1;
                }

                mant &= 0x03ffu;
                uint32_t exp32 = static_cast<uint32_t>(e + 127);
                outBits = sign | (exp32 << 23) | (mant << 13);
            }
        }
        else if (exp == 0x1fu)
        {
            uint32_t exp32 = 0xffu;
            outBits = sign | (exp32 << 23) | (mant << 13);
        }
        else
        {
            uint32_t exp32 = exp + (127 - 15);
            outBits = sign | (exp32 << 23) | (mant << 13);
        }

        float result;
        memcpy(&result, &outBits, sizeof(result));
        return result;
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
    // image.imageType = VK_IMAGE_TYPE_1D;
    // if (m_parms.height > 1)
    //{
    image.imageType = VK_IMAGE_TYPE_2D;
    //}
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
    // imageView.viewType = VK_IMAGE_VIEW_TYPE_1D;
    // if (m_parms.height > 1)
    //{
    imageView.viewType = VK_IMAGE_VIEW_TYPE_2D;
    //}
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

    if (oldLayout <= (VkImageLayout) 0)
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

void Image::ChangeLayout(VkImageLayout newLayout) { m_vkImageLayout = newLayout; }


bool Image::ReadPixelRGToCPU(DeviceContext *device, int cmdBufferIdx, float out[2])
{
    if (!device || m_vkImage == VK_NULL_HANDLE)
    {
        return false;
    }

    const uint32_t width = static_cast<uint32_t>(m_parms.width);
    const uint32_t height = static_cast<uint32_t>(m_parms.height);
    if (width == 0 || height == 0)
    {
        return false;
    }

    VkFormat fmt = m_parms.format;
    VkDeviceSize pixelBytes = 0;
    bool isHalf = false;

    if (fmt == VK_FORMAT_R32G32_SFLOAT)
    {
        pixelBytes = 2 * sizeof(float);
    }
    else if (fmt == VK_FORMAT_R16G16_SFLOAT)
    {
        pixelBytes = 2 * sizeof(uint16_t);
        isHalf = true;
    }
    else
    {
        return false;
    }

    VkDevice dev = device->m_vkDevice;
    VkDeviceSize bufSize = pixelBytes * static_cast<VkDeviceSize>(width) * static_cast<VkDeviceSize>(height);

    // create staging buffer
    VkBufferCreateInfo bufCI{};
    bufCI.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufCI.size = bufSize;
    bufCI.usage = VK_BUFFER_USAGE_TRANSFER_DST_BIT;
    bufCI.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkBuffer stagingBuf = VK_NULL_HANDLE;
    if (vkCreateBuffer(dev, &bufCI, nullptr, &stagingBuf) != VK_SUCCESS)
    {
        return false;
    }

    VkMemoryRequirements memReq = {};
    vkGetBufferMemoryRequirements(dev, stagingBuf, &memReq);

    VkMemoryAllocateInfo alloc = {};
    alloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    alloc.allocationSize = memReq.size;
    alloc.memoryTypeIndex = device->FindMemoryTypeIndex(memReq.memoryTypeBits, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);

    VkDeviceMemory stagingMem = VK_NULL_HANDLE;
    if (vkAllocateMemory(dev, &alloc, nullptr, &stagingMem) != VK_SUCCESS)
    {
        vkDestroyBuffer(dev, stagingBuf, nullptr);
        return false;
    }

    vkBindBufferMemory(dev, stagingBuf, stagingMem, 0);

    // record commands
    VkCommandBuffer cmd = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
    // VkCommandBuffer cmd = device->m_vkCommandBuffers[cmdBufferIdx];

    vkResetCommandBuffer(cmd, 0);
    VkCommandBufferBeginInfo beginInfo = {};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
    vkBeginCommandBuffer(cmd, &beginInfo);

    // image barrier
    VkImageMemoryBarrier barrierToCopy = {};
    barrierToCopy.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrierToCopy.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierToCopy.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
    barrierToCopy.image = m_vkImage;
    barrierToCopy.subresourceRange.baseMipLevel = 0;
    barrierToCopy.subresourceRange.levelCount = 1;
    barrierToCopy.subresourceRange.baseArrayLayer = 0;
    barrierToCopy.subresourceRange.layerCount = 1;
    barrierToCopy.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;

    VkImageLayout oldLayout = m_vkImageLayout;
    if (oldLayout == (VkImageLayout) 0)
    {
        oldLayout = VK_IMAGE_LAYOUT_GENERAL;
    }
    barrierToCopy.oldLayout = oldLayout;
    barrierToCopy.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;

    barrierToCopy.srcAccessMask = VK_ACCESS_MEMORY_READ_BIT | VK_ACCESS_MEMORY_WRITE_BIT | VK_ACCESS_NONE;
    barrierToCopy.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierToCopy);

    // copy region
    VkBufferImageCopy region = {};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {width, height, 1};

    vkCmdCopyImageToBuffer(cmd, m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, stagingBuf, 1, &region);

    // barrier back to original layout
    VkImageMemoryBarrier barrierBack = barrierToCopy;
    barrierBack.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
    barrierBack.dstAccessMask = VK_ACCESS_SHADER_READ_BIT | VK_ACCESS_SHADER_WRITE_BIT;
    barrierBack.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
    barrierBack.newLayout = oldLayout;

    vkCmdPipelineBarrier(cmd, VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_ALL_COMMANDS_BIT, 0, 0, nullptr, 0, nullptr, 1, &barrierBack);

    vkEndCommandBuffer(cmd);

    // submit and wait
    VkFenceCreateInfo fci = {};
    fci.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    VkFence fence = VK_NULL_HANDLE;
    vkCreateFence(dev, &fci, nullptr, &fence);

    VkSubmitInfo submit = {};
    submit.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submit.commandBufferCount = 1;
    submit.pCommandBuffers = &cmd;

    vkQueueSubmit(device->m_vkGraphicsQueue, 1, &submit, fence);
    vkWaitForFences(dev, 1, &fence, VK_TRUE, UINT64_MAX);

    // map staging and read
    void *mapped = nullptr;
    vkMapMemory(dev, stagingMem, 0, bufSize, 0, &mapped);

    if (isHalf)
    {
        uint16_t *h = reinterpret_cast<uint16_t *>(mapped);
        float f0 = ElecNeko::HalfToFloat(h[0]);
        float f1 = ElecNeko::HalfToFloat(h[1]);
        out[0] = f0;
        out[1] = f1;
    }
    else
    {
        float *f = reinterpret_cast<float *>(mapped);
        out[0] = f[0];
        out[1] = f[1];
    }

    vkUnmapMemory(dev, stagingMem);

    // clean up
    vkDestroyFence(dev, fence, nullptr);
    vkFreeCommandBuffers(dev, device->m_vkCommandPool, 1, &cmd);
    vkDestroyBuffer(dev, stagingBuf, nullptr);
    vkFreeMemory(dev, stagingMem, nullptr);

    m_vkImageLayout = oldLayout;

    return true;
}


namespace ElecNeko
{
    bool CubeImage::Create(DeviceContext *device, const int width, const int height, const int mipLevels, VkFormat format)
    {
        assert(width > 0 && height > 0 && width == height);

        m_width = width;
        m_height = height;
        m_mipLevels = mipLevels > 0 ? mipLevels : 1;
        m_format = format;

        // create image
        VkImageCreateInfo imageInfo = {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.flags = VK_IMAGE_CREATE_CUBE_COMPATIBLE_BIT;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.format = m_format;
        imageInfo.extent.width = static_cast<uint32_t>(m_width);
        imageInfo.extent.height = static_cast<uint32_t>(m_height);
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = static_cast<uint32_t>(m_mipLevels);
        imageInfo.arrayLayers = 6; // 6 faces
        imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
        imageInfo.tiling = VK_IMAGE_TILING_OPTIMAL;
        // usage: color attachment (for render-to-face), sampled (for shader read),
        // transfer src/dst for blit/copy if needed
        imageInfo.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_STORAGE_BIT;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        if (vkCreateImage(device->m_vkDevice, &imageInfo, nullptr, &m_vkImage) != VK_SUCCESS)
        {
            printf("ERROR: Failed to create cube image!\n");
            assert(0);
            return false;
        }

        // allocate memory for images
        VkMemoryRequirements memReqs;
        vkGetImageMemoryRequirements(device->m_vkDevice, m_vkImage, &memReqs);

        VkMemoryAllocateInfo memAlloc{};
        memAlloc.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        memAlloc.allocationSize = memReqs.size;
        memAlloc.memoryTypeIndex = device->FindMemoryTypeIndex(memReqs.memoryTypeBits, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

        if (vkAllocateMemory(device->m_vkDevice, &memAlloc, nullptr, &m_vkDeviceMemory) != VK_SUCCESS)
        {
            printf("ERROR: Failed to allocate cube image memory!\n");
            assert(0);
            return false;
        }

        if (vkBindImageMemory(device->m_vkDevice, m_vkImage, m_vkDeviceMemory, 0) != VK_SUCCESS)
        {
            printf("ERROR: Failed to bind cube image memory!\n");
            assert(0);
            return false;
        }

        // create cube view for sampling
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = m_vkImage;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
        viewInfo.format = m_format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = static_cast<uint32_t>(m_mipLevels);
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 6; // 6 faces

        if (vkCreateImageView(device->m_vkDevice, &viewInfo, nullptr, &m_vkImageView) != VK_SUCCESS)
        {
            printf("ERROR: Failed to create cube image view!\n");
            assert(0);
            return false;
        }

        // create per-face-per-mip views for rendering
        m_faceMipViews.clear();
        m_faceMipViews.resize(m_mipLevels, VK_NULL_HANDLE);

        for (int mip = 0; mip < m_mipLevels; ++mip)
        {
            VkImageViewCreateInfo faceViewInfo{};
            faceViewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            faceViewInfo.image = m_vkImage;
            faceViewInfo.viewType = VK_IMAGE_VIEW_TYPE_CUBE;
            faceViewInfo.format = m_format;
            faceViewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            faceViewInfo.subresourceRange.baseMipLevel = mip;
            faceViewInfo.subresourceRange.levelCount = 1;
            faceViewInfo.subresourceRange.baseArrayLayer = 0;
            faceViewInfo.subresourceRange.layerCount = 6;

            VkImageView faceView = VK_NULL_HANDLE;
            if (vkCreateImageView(device->m_vkDevice, &faceViewInfo, nullptr, &faceView) != VK_SUCCESS)
            {
                printf("ERROR: Failed to create cube face image view!\n");
                assert(0);
                // cleanup previously created views
                for (VkImageView v: m_faceMipViews)
                {
                    if (v != VK_NULL_HANDLE)
                    {
                        vkDestroyImageView(device->m_vkDevice, v, nullptr);
                    }
                }
                vkDestroyImageView(device->m_vkDevice, m_vkImageView, nullptr);
                vkDestroyImage(device->m_vkDevice, m_vkImage, nullptr);
                vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
                m_faceMipViews.clear();
                return false;
            }
            m_faceMipViews[mip] = faceView;
        }

        m_vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        m_mipLayouts.resize(m_mipLevels, VK_IMAGE_LAYOUT_UNDEFINED);
        return true;
    }

    void CubeImage::Cleanup(DeviceContext *device)
    {
        for (VkImageView v: m_faceMipViews)
        {
            if (v != VK_NULL_HANDLE)
            {
                vkDestroyImageView(device->m_vkDevice, v, nullptr);
            }
        }
        m_faceMipViews.clear();

        if (m_vkImageView != VK_NULL_HANDLE)
        {
            vkDestroyImageView(device->m_vkDevice, m_vkImageView, nullptr);
            m_vkImageView = VK_NULL_HANDLE;
        }

        if (m_vkImage != VK_NULL_HANDLE)
        {
            vkDestroyImage(device->m_vkDevice, m_vkImage, nullptr);
            m_vkImage = VK_NULL_HANDLE;
        }

        if (m_vkDeviceMemory != VK_NULL_HANDLE)
        {
            vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
            m_vkDeviceMemory = VK_NULL_HANDLE;
        }

        m_vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    }

    void CubeImage::TransitionLayout(DeviceContext *device, VkImageLayout newLayout)
    {
        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);
        TransitionLayout(cmdBuffer, newLayout);
        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);
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

    void CubeImage::TransitionMipLayout(VkCommandBuffer cmdBuffer, int mip, VkImageLayout newLayout)
    {
        if (mip < 0 || mip >= m_mipLevels)
        {
            return;
        }

        VkImageMemoryBarrier barrier = {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = m_mipLayouts[mip];
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = m_vkImage;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = mip;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 6;

        LayoutBarrierInfo srcInfo = GetStageAccessForLayout(m_mipLayouts[mip]);
        LayoutBarrierInfo dstInfo = GetStageAccessForLayout(newLayout);
        barrier.srcAccessMask = srcInfo.access;
        barrier.dstAccessMask = dstInfo.access;
        VkPipelineStageFlags sourceStage = srcInfo.stage;
        VkPipelineStageFlags destinationStage = dstInfo.stage;

        vkCmdPipelineBarrier(cmdBuffer, sourceStage, destinationStage, 0, 0, nullptr, 0, nullptr, 1, &barrier);

        m_mipLayouts[mip] = newLayout;
    }

    void CubeImage::ChangeLayout(VkImageLayout newLayout) { m_vkImageLayout = newLayout; }


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

        VkImageLayout oldLayout = m_vkImageLayout;

        if (oldLayout <= (VkImageLayout) 0)
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
