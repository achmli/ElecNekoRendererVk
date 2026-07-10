#include "RHI2/Vulkan/VulkanContext.h"

#include "RHI/DeviceContext.h"

#include <cassert>

namespace RHI
{
    VulkanContext::VulkanContext(DeviceContext *legacyDevice) : m_legacyDevice(legacyDevice)
    {
        assert(m_legacyDevice != nullptr);

        m_instance = m_legacyDevice->m_vkInstance;
        m_physicalDevice = m_legacyDevice->m_vkPhysicalDevice;
        m_device = m_legacyDevice->m_vkDevice;
        m_graphicsQueue = m_legacyDevice->m_vkGraphicsQueue;
        m_commandPool = m_legacyDevice->m_vkCommandPool;

        assert(m_instance != VK_NULL_HANDLE);
        assert(m_physicalDevice != VK_NULL_HANDLE);
        assert(m_device != VK_NULL_HANDLE);
        assert(m_graphicsQueue != VK_NULL_HANDLE);
        assert(m_commandPool != VK_NULL_HANDLE);
    }

    VkDevice VulkanContext::GetVkDevice() const { return m_device; }

    VkPhysicalDevice VulkanContext::GetVkPhysicalDevice() const { return m_physicalDevice; }

    VkQueue VulkanContext::GetGraphicsQueue() const { return m_graphicsQueue; }

    VkCommandPool VulkanContext::GetCommandPool() const { return m_commandPool; }

    VkInstance VulkanContext::GetVkInstance() const { return m_instance; }

    uint32_t VulkanContext::FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties) const
    {
        VkPhysicalDevice physicalDevice = GetVkPhysicalDevice();

        assert(physicalDevice != VK_NULL_HANDLE);

        VkPhysicalDeviceMemoryProperties memoryProperties{};
        vkGetPhysicalDeviceMemoryProperties(physicalDevice, &memoryProperties);

        for (uint32_t memoryTypeIndex = 0; memoryTypeIndex < memoryProperties.memoryTypeCount; ++memoryTypeIndex)
        {
            const bool typeMatches = (typeFilter & (1u << memoryTypeIndex)) != 0;

            const bool propertyMatches = (memoryProperties.memoryTypes[memoryTypeIndex].propertyFlags & properties) == properties;

            if (typeMatches && propertyMatches)
            {
                return memoryTypeIndex;
            }
        }

        assert(false && "Failed to find suitable Vulkan memory type.");
        return 0;
    }

    VkCommandBuffer VulkanContext::BeginSingleTimeCommands()
    {
        VkDevice vkDevice = GetVkDevice();

        VkCommandPool commandPool = GetCommandPool();

        assert(vkDevice != VK_NULL_HANDLE);
        assert(commandPool != VK_NULL_HANDLE);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer = VK_NULL_HANDLE;

        VkResult result = vkAllocateCommandBuffers(vkDevice, &allocInfo, &commandBuffer);

        assert(result == VK_SUCCESS);
        assert(commandBuffer != VK_NULL_HANDLE);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        result = vkBeginCommandBuffer(commandBuffer, &beginInfo);

        assert(result == VK_SUCCESS);

        return commandBuffer;
    }

    void VulkanContext::EndSingleTimeCommands(VkCommandBuffer commandBuffer)
    {
        if (commandBuffer == VK_NULL_HANDLE)
        {
            return;
        }

        VkDevice vkDevice = GetVkDevice();

        VkQueue graphicsQueue = GetGraphicsQueue();

        VkCommandPool commandPool = GetCommandPool();

        assert(vkDevice != VK_NULL_HANDLE);
        assert(graphicsQueue != VK_NULL_HANDLE);
        assert(commandPool != VK_NULL_HANDLE);

        VkResult result = vkEndCommandBuffer(commandBuffer);

        assert(result == VK_SUCCESS);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        result = vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

        assert(result == VK_SUCCESS);

        result = vkQueueWaitIdle(graphicsQueue);

        assert(result == VK_SUCCESS);

        vkFreeCommandBuffers(vkDevice, commandPool, 1, &commandBuffer);
    }

    void VulkanContext::SetObjectName(uint64_t objectHandle, VkObjectType objectType, const char *name)
    {
        if (name == nullptr || objectHandle == 0 || GetVkDevice() == VK_NULL_HANDLE)
        {
            return;
        }

        PFN_vkSetDebugUtilsObjectNameEXT setDebugUtilsObjectName =
                reinterpret_cast<PFN_vkSetDebugUtilsObjectNameEXT>(vkGetDeviceProcAddr(GetVkDevice(), "vkSetDebugUtilsObjectNameEXT"));

        if (setDebugUtilsObjectName == nullptr)
        {
            return;
        }

        VkDebugUtilsObjectNameInfoEXT nameInfo{};
        nameInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_OBJECT_NAME_INFO_EXT;
        nameInfo.objectType = objectType;
        nameInfo.objectHandle = objectHandle;
        nameInfo.pObjectName = name;

        setDebugUtilsObjectName(GetVkDevice(), &nameInfo);
    }

    void VulkanContext::EnqueueDeferredDelete(std::function<void()> deleter)
    {
        if (!deleter)
        {
            return;
        }

        m_deferredDeletes.push_back(std::move(deleter));
    }

    void VulkanContext::FlushDeferredDeletes()
    {
        for (std::function<void()> &deleter: m_deferredDeletes)
        {
            if (deleter)
            {
                deleter();
            }
        }

        m_deferredDeletes.clear();
    }
} // namespace RHI
