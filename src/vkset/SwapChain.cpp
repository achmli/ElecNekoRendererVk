#include "SwapChain.hpp"
#include "DeviceContext.hpp"

#include <cassert>

void SwapChain::Resize(DeviceContext* device, int width, int height)
{
    vkDeviceWaitIdle(device->vkDevice);

    // delete swap chain
    Cleanup(device);

    // create swap chain
    Create(device,width,height);
}

void SwapChain::Cleanup(DeviceContext* device)
{
    // semaphores
    vkDestroySemaphore(device->vkDevice,vkRenderFinishedSemaphore,nullptr);
    vkDestroySemaphore(device->vkDevice,vkImageAvailableSemaphore,nullptr);

    // depth buffer
    vkDestroyImageView(device->vkDevice,vkDepthImageView,nullptr);
    vkDestroyImage(device->vkDevice,vkDepthImage,nullptr);
    vkFreeMemory(device->vkDevice,vkDepthImageMemory,nullptr);

    // frame buffer
    for(const auto& frameBuffer:vkFramebuffers)
    {
        vkDestroyFramebuffer(device->vkDevice,frameBuffer,nullptr);
    }

    // render pass
    vkDestroyRenderPass(device->vkDevice,vkRenderPass,nullptr);

    // color buffers
    for(const auto& colorImageView:vkImageViews)
    {
        vkDestroyImageView(device->vkDevice,colorImageView,nullptr);
    }

    // swap chain
    vkDestroySwapchainKHR(device->vkDevice,vkSwapChain,nullptr);
}

bool SwapChain::Create(DeviceContext* device, int width, int height)
{
    VkResult result;

    windowWidth=width;
    windowHeight=height;

    vkExt.width=width;
    vkExt.height=height;

    // Create Semaphores
    {
        VkSemaphoreCreateInfo createInfo={};
        createInfo.sType=VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        result=vkCreateSemaphore(device->vkDevice,&createInfo,nullptr,&vkImageAvailableSemaphore);
        if(result!=VK_SUCCESS)
        {
            std::printf("Error: Failed to create image available semaphore!\n");
            assert(0);
            return false;
        }

        result=vkCreateSemaphore(device->vkDevice,&createInfo,nullptr,&vkRenderFinishedSemaphore);
        if(result!=VK_SUCCESS)
        {
            std::printf("Error: Failed to create render finished semaphore!\n");
            assert(0);
            return false;
        }
    }

    // Create swap chain
    {
        const int deviceIndex=device->deviceIndex;
        PhysicalDeviceProperties physicalDeviceInfo=device->vkPhysicalDevices[deviceIndex];

        VkSurfaceFormatKHR surfaceFormat;
        surfaceFormat.format=VK_FORMAT_B8G8R8A8_UNORM;
        surfaceFormat.colorSpace=VK_COLOR_SPACE_SRGB_NONLINEAR_KHR;

        VkPresentModeKHR presentMode=VK_PRESENT_MODE_FIFO_KHR;
        for(const auto& vkPresentMode:physicalDeviceInfo.vkPresentModes)
        {
            if(vkPresentMode==VK_PRESENT_MODE_MAILBOX_KHR)
            {
                presentMode=VK_PRESENT_MODE_MAILBOX_KHR;
                break;
            }
        }

        uint32_t imageCount=physicalDeviceInfo.vkSurfaceCapabilities.minImageCount+1;
        if(physicalDeviceInfo.vkSurfaceCapabilities.maxImageCount>0&&imageCount>physicalDeviceInfo.vkSurfaceCapabilities.maxImageCount)
        {
            imageCount=physicalDeviceInfo.vkSurfaceCapabilities.maxImageCount;
        }

        uint32_t queueFamilyIndices[]
        {
            (uint32_t)device->graphicsFamilyIdx,
            (uint32_t)device->presentFamilyIdx
        };

        VkSwapchainCreateInfoKHR scCreateInfo={};
        scCreateInfo.sType=VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        scCreateInfo.surface=device->vkSurface;
        scCreateInfo.minImageCount=imageCount;
        scCreateInfo.imageFormat=surfaceFormat.format;
        scCreateInfo.imageColorSpace=surfaceFormat.colorSpace;
        scCreateInfo.imageExtent=vkExt;
        scCreateInfo.imageArrayLayers=1;
        scCreateInfo.imageUsage=VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;
        scCreateInfo.queueFamilyIndexCount=0;
        scCreateInfo.pQueueFamilyIndices=nullptr;
        scCreateInfo.imageSharingMode=VK_SHARING_MODE_EXCLUSIVE;
        if(device->graphicsFamilyIdx!=device->presentFamilyIdx)
        {
            scCreateInfo.imageSharingMode=VK_SHARING_MODE_CONCURRENT;
            scCreateInfo.queueFamilyIndexCount=2;
            scCreateInfo.pQueueFamilyIndices=queueFamilyIndices;
        }
        scCreateInfo.preTransform=physicalDeviceInfo.vkSurfaceCapabilities.currentTransform;
        // todo: maybe it may needs to be something else
        scCreateInfo.compositeAlpha=VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        scCreateInfo.presentMode=presentMode;
        scCreateInfo.clipped=VK_TRUE;

        result=vkCreateSwapchainKHR(device->vkDevice,&scCreateInfo,nullptr,&vkSwapChain);
        if(result!=VK_SUCCESS)
        {
            std::printf("Error: Failed to create swap chain!\n");
            assert(0);
            return false;
        }

        vkGetSwapchainImagesKHR(device->vkDevice,vkSwapChain,&imageCount,nullptr);
        vkColorImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device->vkDevice,vkSwapChain,&imageCount,vkColorImages.data());

        vkColorImageFormat=surfaceFormat.format;
    }

    // Create image views for swap chain
    {
        vkImageViews.resize(vkColorImages.size());

        for(uint32_t i=0;i<vkColorImages.size();i++)
        {
            VkImageViewCreateInfo createInfo={};
            createInfo.sType=VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image=vkColorImages[i];
            createInfo.viewType=VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format=vkColorImageFormat;
            createInfo.subresourceRange.aspectMask=VK_IMAGE_ASPECT_COLOR_BIT;
            // todo: maybe we need more
            createInfo.subresourceRange.baseMipLevel=0;
            createInfo.subresourceRange.levelCount=1;
            createInfo.subresourceRange.baseArrayLayer=0;
            createInfo.subresourceRange.layerCount=1;

            result=vkCreateImageView(device->vkDevice,&createInfo,nullptr,&vkImageViews[i]);
            if(result!=VK_SUCCESS)
            {
                std::printf("Error: Failed to create Image View!\n");
                assert(0);
                return false;
            }
        }
    }

    // create render pass for swap chain
    {
        VkAttachmentDescription colorAttachmet={};
        colorAttachmet.format=vkColorImageFormat;
        colorAttachmet.samples=VK_SAMPLE_COUNT_1_BIT;
        colorAttachmet.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmet.storeOp=VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmet.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmet.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmet.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmet.finalLayout=VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentDescription depthAttachment={};
        depthAttachment.format=vkDepthFormat;
        depthAttachment.samples=VK_SAMPLE_COUNT_1_BIT;
        depthAttachment.loadOp=VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp=VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp=VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout=VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkAttachmentReference colorRef={};
        colorRef.attachment=0;
        colorRef.layout=VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkAttachmentReference depthRef={};
        depthRef.attachment=1;
        depthRef.layout=VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass={};
        subpass.pipelineBindPoint=VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount=1;
        subpass.pColorAttachments=&colorRef;
        subpass.pDepthStencilAttachment=&depthRef;

        VkSubpassDependency dependency={};
        dependency.srcSubpass=VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass=0;
        dependency.srcStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstStageMask=VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask=0;
        dependency.dstAccessMask=VK_ACCESS_COLOR_ATTACHMENT_READ_BIT | VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkAttachmentDescription attachments[2]=
        {
            colorAttachmet,
            depthAttachment
        };

        VkRenderPassCreateInfo createInfo={};
        createInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        createInfo.attachmentCount=2;
        createInfo.pAttachments=attachments;
        createInfo.subpassCount=1;
        createInfo.pSubpasses=&subpass;createInfo.dependencyCount=1;
        createInfo.pDependencies=&dependency;

        result=vkCreateRenderPass(device->vkDevice,&createInfo,nullptr,&vkRenderPass);
        if(result!=VK_SUCCESS)
        {
            std::printf("Error: Failed to create render pass!\n");
            assert(0);
            return false;
        }
    }

    // create frame buffers for swap chain
    {
        vkFramebuffers.resize(vkImageViews.size());
        for(size_t i=0;i<vkImageViews.size();i++)
        {
            VkImageView attachments[2]=
            {
                vkImageViews[i],
                vkDepthImageView
            };

            VkFramebufferCreateInfo createInfo={};
            createInfo.sType=VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            createInfo.renderPass=vkRenderPass;
            createInfo.attachmentCount=2;
            createInfo.pAttachments=attachments;
            createInfo.width=vkExt.width;
            createInfo.height=vkExt.height;
            createInfo.layers=1;

            result=vkCreateFramebuffer(device->vkDevice,&createInfo,nullptr,&vkFramebuffers[i]);
            if(result!=VK_SUCCESS)
            {
                std::printf("Error: Failed to craete frame buffer!\n");
                assert(0);
                return false;
            }
        }
    }

    return true;
}

uint32_t SwapChain::BeginFrame(DeviceContext* device)
{
    VkResult result;

    currentImageIndex=0;

    result=vkAcquireNextImageKHR(device->vkDevice,vkSwapChain,std::numeric_limits<uint64_t>::max(),vkImageAvailableSemaphore,VK_NULL_HANDLE,&currentImageIndex);
    if(result!=VK_SUCCESS&&result!=VK_SUBOPTIMAL_KHR)
    {
        std::printf("Error: Failed to acquire swap chain!\n");
        assert(0);
    }

    // Reset the command buffer
    vkResetCommandBuffer(device->vkCommandBuffers[currentImageIndex],VK_COMMAND_BUFFER_RESET_RELEASE_RESOURCES_BIT);


    // Begin recording draw command
    VkCommandBufferBeginInfo beginInfo={};
    beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags=VK_COMMAND_BUFFER_USAGE_SIMULTANEOUS_USE_BIT;
    vkBeginCommandBuffer(device->vkCommandBuffers[currentImageIndex],&beginInfo);

    return currentImageIndex;
}

void SwapChain::EndFrame(DeviceContext* device)
{
    VkResult result;

    result=vkEndCommandBuffer(device->vkCommandBuffers[currentImageIndex]);
    if(result!=VK_SUCCESS)
    {
        std::printf("Error: Failed to record command buffer!\n");
        assert(0);
    }

    VkPipelineStageFlags waitStages[]=
    {
        VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT
    };

    VkSubmitInfo submitInfo={};
    submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.waitSemaphoreCount=1;
    submitInfo.pWaitSemaphores=&vkImageAvailableSemaphore;
    submitInfo.pWaitDstStageMask=waitStages;
    submitInfo.commandBufferCount=1;
    submitInfo.pCommandBuffers=&device->vkCommandBuffers[currentImageIndex];
    submitInfo.signalSemaphoreCount=1;
    submitInfo.pSignalSemaphores=&vkRenderFinishedSemaphore;

    result=vkQueueSubmit(device->vkGraphicsQueue,1,&submitInfo,VK_NULL_HANDLE);
    if(result!=VK_SUCCESS)
    {
        std::printf("Error: Failed to submit queue!\n");
        assert(0);
    }

    VkPresentInfoKHR presentInfo={};
    presentInfo.sType=VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount=1;
    presentInfo.pWaitSemaphores=&vkRenderFinishedSemaphore;
    presentInfo.swapchainCount=1;
    presentInfo.pSwapchains=&vkSwapChain;
    presentInfo.pImageIndices=&currentImageIndex;

    result=vkQueuePresentKHR(device->vkPresentQueue,&presentInfo);
    if(result!=VK_SUCCESS&&result!=VK_SUBOPTIMAL_KHR)
    {
        std::printf("Error: Failed to present!\n");
        assert(0);
    }

    vkQueueWaitIdle(device->vkPresentQueue);
}

void SwapChain::BeginRenderPass(DeviceContext* device)
{
    // Set Render pass
    VkClearValue clearValues[2];
    clearValues[0].color={0.f,0.f,0.f,1.f};
    clearValues[1].depthStencil={1.f,0};

    VkRenderPassBeginInfo beginInfo={};
    beginInfo.sType=VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    beginInfo.renderPass=vkRenderPass;
    beginInfo.framebuffer=vkFramebuffers[currentImageIndex];
    beginInfo.renderArea.offset={0,0};
    beginInfo.renderArea.extent=vkExt;
    beginInfo.clearValueCount=2;
    beginInfo.pClearValues=clearValues;

    vkCmdBeginRenderPass(device->vkCommandBuffers[currentImageIndex],&beginInfo,VK_SUBPASS_CONTENTS_INLINE);

    // Set the viewports
    VkViewport viewport={};
    viewport.x=0.f;
    viewport.y=0.f;
    viewport.width=static_cast<float>(windowWidth);
    viewport.height=static_cast<float>(windowHeight);
    viewport.minDepth=0.f;
    viewport.maxDepth=1.f;
    vkCmdSetViewport(device->vkCommandBuffers[currentImageIndex],0,1,&viewport);

    VkRect2D scissor={};
    scissor.offset.x=0;
    scissor.offset.y=0;
    scissor.extent.width=windowWidth;
    scissor.extent.height=windowHeight;
    vkCmdSetScissor(device->vkCommandBuffers[currentImageIndex],0,1,&scissor);
}

void SwapChain::EndRenderPass(DeviceContext* device)
{
    vkCmdEndRenderPass(device->vkCommandBuffers[currentImageIndex]);
}