#pragma once

#include "RHI/Buffer.h"
#include "RHI2/RHIBuffer.h"

#include <vulkan/vulkan.h>

namespace RHI
{
    class VulkanContext;

    class VulkanBuffer final : public Buffer
    {
    public:
        VulkanBuffer(VulkanContext *context, const BufferDesc &desc);
        ~VulkanBuffer() override;

        bool Create();
        void Destroy();

        const BufferDesc &GetDesc() const override;
        uint64_t GetSize() const override;

        void *Map() override;
        void Unmap() override;

        VkBuffer GetVkBuffer() const;
        VkDeviceMemory GetVkMemory() const;

        // Transition helper for old descriptor binding code.
        // This is a non-owning legacy Buffer view over the RHI2-owned VkBuffer.
        ::Buffer *GetLegacyBufferForTransition();
        const ::Buffer *GetLegacyBufferForTransition() const;

    private:
        VkBufferUsageFlags TranslateUsage(BufferUsage usage) const;
        VkMemoryPropertyFlags GetMemoryPropertyFlags() const;

        void RefreshLegacyView();

    private:
        VulkanContext *m_context = nullptr;
        BufferDesc m_desc{};

        VkBuffer m_buffer = VK_NULL_HANDLE;
        VkDeviceMemory m_memory = VK_NULL_HANDLE;
        VkDeviceSize m_allocationSize = 0;
        VkMemoryPropertyFlags m_memoryPropertyFlags = 0;

        void *m_mapped = nullptr;

        // Non-owning compatibility view. Do not call Cleanup() on this object.
        ::Buffer m_legacyView;
    };
} // namespace RHI
