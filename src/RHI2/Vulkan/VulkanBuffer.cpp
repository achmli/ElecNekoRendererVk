#include "RHI2/Vulkan/VulkanBuffer.h"

#include "RHI2/Vulkan/VulkanContext.h"

#include <cassert>

namespace RHI
{
    VulkanBuffer::VulkanBuffer(VulkanContext *context, const BufferDesc &desc) : m_context(context), m_desc(desc) {}

    VulkanBuffer::~VulkanBuffer() { Destroy(); }

    VkBufferUsageFlags VulkanBuffer::TranslateUsage(BufferUsage usage) const
    {
        VkBufferUsageFlags flags = 0;

        if (HasFlag(usage, BufferUsage::Vertex))
        {
            flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        }

        if (HasFlag(usage, BufferUsage::Index))
        {
            flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        }

        if (HasFlag(usage, BufferUsage::Uniform))
        {
            flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
        }

        if (HasFlag(usage, BufferUsage::Storage))
        {
            flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
        }

        if (HasFlag(usage, BufferUsage::TransferSrc))
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        }

        if (HasFlag(usage, BufferUsage::TransferDst))
        {
            flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
        }

        return flags;
    }

    VkMemoryPropertyFlags VulkanBuffer::GetMemoryPropertyFlags() const
    {
        if (m_desc.cpuVisible)
        {
            return VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        }

        return VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
    }

    void VulkanBuffer::RefreshLegacyView()
    {
        m_legacyView.m_vkBuffer = m_buffer;
        m_legacyView.m_vkBufferMemory = m_memory;
        m_legacyView.m_vkBufferSize = static_cast<VkDeviceSize>(m_desc.size);
        m_legacyView.m_vkMemoryPropertyFlags = m_memoryPropertyFlags;
        m_legacyView.m_offset = 0;
    }

    bool VulkanBuffer::Create()
    {
        if (m_desc.size == 0)
        {
            return false;
        }

        Destroy();

        const VkBufferUsageFlags usageFlags = TranslateUsage(m_desc.usage);

        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.pNext = nullptr;
        bufferInfo.flags = 0;
        bufferInfo.size = static_cast<VkDeviceSize>(m_desc.size);
        bufferInfo.usage = usageFlags;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
        bufferInfo.queueFamilyIndexCount = 0;
        bufferInfo.pQueueFamilyIndices = nullptr;

        VkResult result = vkCreateBuffer(m_context->GetVkDevice(), &bufferInfo, nullptr, &m_buffer);

        if (result != VK_SUCCESS)
        {
            m_buffer = VK_NULL_HANDLE;
            RefreshLegacyView();
            return false;
        }

        VkMemoryRequirements memoryRequirements{};
        vkGetBufferMemoryRequirements(m_context->GetVkDevice(), m_buffer, &memoryRequirements);

        m_memoryPropertyFlags = GetMemoryPropertyFlags();
        m_allocationSize = memoryRequirements.size;

        VkMemoryAllocateInfo allocateInfo{};
        allocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocateInfo.pNext = nullptr;
        allocateInfo.allocationSize = memoryRequirements.size;
        allocateInfo.memoryTypeIndex = m_context->FindMemoryTypeIndex(memoryRequirements.memoryTypeBits, m_memoryPropertyFlags);

        result = vkAllocateMemory(m_context->GetVkDevice(), &allocateInfo, nullptr, &m_memory);

        // printf("[RHI2VulkanBufferCreate] bufferInfo.pNext=%p allocateInfo.pNext=%p size=%llu usage=0x%x memoryFlags=0x%x\n", bufferInfo.pNext,
        //        allocateInfo.pNext, static_cast<unsigned long long>(m_desc.size), static_cast<unsigned int>(usageFlags),
        //        static_cast<unsigned int>(m_memoryPropertyFlags));

        if (result != VK_SUCCESS)
        {
            vkDestroyBuffer(m_context->GetVkDevice(), m_buffer, nullptr);
            m_buffer = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_allocationSize = 0;
            m_memoryPropertyFlags = 0;
            RefreshLegacyView();
            return false;
        }

        result = vkBindBufferMemory(m_context->GetVkDevice(), m_buffer, m_memory, 0);

        if (result != VK_SUCCESS)
        {
            Destroy();
            return false;
        }

        RefreshLegacyView();

        if (m_desc.debugName != nullptr)
        {
            m_context->SetObjectName(reinterpret_cast<uint64_t>(m_buffer), VK_OBJECT_TYPE_BUFFER, m_desc.debugName);
        }

        return true;
    }

    void VulkanBuffer::Destroy()
    {
        if (m_context == nullptr)
        {
            m_buffer = VK_NULL_HANDLE;
            m_memory = VK_NULL_HANDLE;
            m_allocationSize = 0;
            m_memoryPropertyFlags = 0;
            m_mapped = nullptr;
            RefreshLegacyView();
            return;
        }

        if (m_mapped != nullptr)
        {
            Unmap();
        }

        VkDevice vkDevice = m_context->GetVkDevice();
        VkBuffer buffer = m_buffer;
        VkDeviceMemory memory = m_memory;

        m_buffer = VK_NULL_HANDLE;
        m_memory = VK_NULL_HANDLE;
        m_allocationSize = 0;
        m_memoryPropertyFlags = 0;

        RefreshLegacyView();

        if (buffer == VK_NULL_HANDLE && memory == VK_NULL_HANDLE)
        {
            return;
        }

        m_context->EnqueueDeferredDelete(
                [vkDevice, buffer, memory]()
                {
                    if (buffer != VK_NULL_HANDLE)
                    {
                        vkDestroyBuffer(vkDevice, buffer, nullptr);
                    }

                    if (memory != VK_NULL_HANDLE)
                    {
                        vkFreeMemory(vkDevice, memory, nullptr);
                    }
                });
    }

    const BufferDesc &VulkanBuffer::GetDesc() const { return m_desc; }

    uint64_t VulkanBuffer::GetSize() const { return m_desc.size; }

    void *VulkanBuffer::Map()
    {
        if (m_context == nullptr || m_memory == VK_NULL_HANDLE)
        {
            return nullptr;
        }

        if (m_mapped != nullptr)
        {
            return m_mapped;
        }

        VkResult result = vkMapMemory(m_context->GetVkDevice(), m_memory, 0, static_cast<VkDeviceSize>(m_desc.size), 0, &m_mapped);

        if (result != VK_SUCCESS)
        {
            m_mapped = nullptr;
            return nullptr;
        }

        return m_mapped;
    }

    void VulkanBuffer::Unmap()
    {
        if (m_context == nullptr || m_memory == VK_NULL_HANDLE || m_mapped == nullptr)
        {
            return;
        }

        vkUnmapMemory(m_context->GetVkDevice(), m_memory);
        m_mapped = nullptr;
    }

    VkBuffer VulkanBuffer::GetVkBuffer() const { return m_buffer; }

    VkDeviceMemory VulkanBuffer::GetVkMemory() const { return m_memory; }

    ::Buffer *VulkanBuffer::GetLegacyBufferForTransition() { return &m_legacyView; }

    const ::Buffer *VulkanBuffer::GetLegacyBufferForTransition() const { return &m_legacyView; }
} // namespace RHI
