#pragma once

#include <functional>
#include <vector>

#include <vulkan/vulkan.h>

class DeviceContext;

namespace RHI
{
    class VulkanContext
    {
    public:
        explicit VulkanContext(DeviceContext *legacyDevice);

        VkDevice GetVkDevice() const;
        VkPhysicalDevice GetVkPhysicalDevice() const;
        VkQueue GetGraphicsQueue() const;
        VkCommandPool GetCommandPool() const;
        VkInstance GetVkInstance() const;

        uint32_t FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const;

        VkCommandBuffer BeginSingleTimeCommands();
        void EndSingleTimeCommands(VkCommandBuffer commandBuffer);

        void SetObjectName(uint64_t objectHandle, VkObjectType objectType, const char *name);

        void EnqueueDeferredDelete(std::function<void()> deleter);
        void FlushDeferredDeletes();

    private:
        DeviceContext *m_legacyDevice = nullptr;

        VkInstance m_instance = VK_NULL_HANDLE;
        VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
        VkDevice m_device = VK_NULL_HANDLE;
        VkQueue m_graphicsQueue = VK_NULL_HANDLE;
        VkCommandPool m_commandPool = VK_NULL_HANDLE;

        std::vector<std::function<void()>> m_deferredDeletes;
    };
} // namespace RHI
