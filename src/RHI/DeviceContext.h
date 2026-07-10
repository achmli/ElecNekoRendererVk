//
//  DeviceContext.h
//
#pragma once
#include <functional>
#include <vector>
#include "RHIBufferRegistry.h"
#include "SwapChain.h"

/*
====================================================
Vulkan Extension Functions
====================================================
*/
class vfs
{
public:
    static void Link(VkInstance instance);
    static void LinkDevice(VkDevice device);

    static PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
    static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
    static PFN_vkSetDebugUtilsObjectNameEXT vkSetDebugUtilsObjectNameEXT;
};

/*
====================================================
PhysicalDeviceProperties
====================================================
*/
class PhysicalDeviceProperties
{
public:
    VkPhysicalDevice m_vkPhysicalDevice;
    VkPhysicalDeviceProperties m_vkDeviceProperties;
    VkPhysicalDeviceMemoryProperties m_vkMemoryProperties;
    VkPhysicalDeviceFeatures m_vkFeatures;
    VkSurfaceCapabilitiesKHR m_vkSurfaceCapabilities;
    std::vector<VkSurfaceFormatKHR> m_vkSurfaceFormats;
    std::vector<VkPresentModeKHR> m_vkPresentModes;
    std::vector<VkQueueFamilyProperties> m_vkQueueFamilyProperties;
    std::vector<VkExtensionProperties> m_vkExtensionProperties;

    bool AcquireProperties(VkPhysicalDevice device, VkSurfaceKHR vkSurface);
    bool HasExtensions(const char **extensions, const int num) const;
};

/*
====================================================
DeviceContext
====================================================
*/
class DeviceContext
{
public:
    bool CreateInstance(bool enableLayers, const std::vector<const char *> &extensions);
    void Cleanup();

    bool m_debugUtilsEnabled = false;
    bool m_enableLayers;
    VkInstance m_vkInstance;
    VkDebugReportCallbackEXT m_vkDebugCallback;

    void SetObjectName(uint64_t objectHandle, VkObjectType objectType, const char *name);

    VkSurfaceKHR m_vkSurface;

    bool CreateDevice();
    bool CreatePhysicalDevice();
    bool CreateLogicalDevice();

    std::vector<PhysicalDeviceProperties> m_physicalDevices;

    //
    //	Device related
    //
    int m_deviceIndex;
    VkPhysicalDevice m_vkPhysicalDevice;
    VkDevice m_vkDevice;

    int m_graphicsFamilyIdx;
    int m_presentFamilyIdx;

    VkQueue m_vkGraphicsQueue;
    VkQueue m_vkPresentQueue;

    uint32_t FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);

    static const std::vector<const char *> m_deviceExtensions;
    std::vector<const char *> m_validationLayers;

    // Command Lists
    RHI::BufferRegistry m_bufferRegistry;

    //
    //	Command Buffers
    //
    bool CreateCommandBuffers();

    VkCommandPool m_vkCommandPool;
    std::vector<VkCommandBuffer> m_vkCommandBuffers;

    VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level);
    void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

    VkCommandBuffer BeginSingleTimeCommands();
    void EndSingleTimeCommands(VkCommandBuffer cmdBuffer);

    //
    //	Swap chain related
    //
    SwapChain m_swapChain;
    bool CreateSwapChain(int width, int height) { return m_swapChain.Create(this, width, height); }
    void ResizeWindow(int width, int height) { m_swapChain.Resize(this, width, height); }

    uint32_t BeginFrame() { return m_swapChain.BeginFrame(this); }
    void EndFrame()
    {
        m_swapChain.EndFrame(this);
        m_frameIndex++;
        ProcessDeferredDeletes();
    }

    void BeginRenderPass() { m_swapChain.BeginRenderPass(this); }
    void EndRenderPass() { m_swapChain.EndRenderPass(this); }


    int GetAligendUniformByteOffset(const int offset) const;

    // deferred deletion
    struct DeferredDeleteItem
    {
        uint64_t executeFrame = 0;
        std::function<void()> deleter;
    };

    void EnqueueDeferredDelete(std::function<void()> deleter);
    void ProcessDeferredDeletes();
    void FlushDeferredDeletes();

    uint64_t m_frameIndex = 0;
    std::vector<DeferredDeleteItem> m_deferredDeletes;
};
