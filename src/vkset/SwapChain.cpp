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
}

