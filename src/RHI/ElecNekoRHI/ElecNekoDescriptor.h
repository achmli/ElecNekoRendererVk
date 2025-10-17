#pragma once

#include <cassert>
#include <iostream>
#include <vector>
#include <vulkan/vulkan.h>
#include "RHI/Buffer.h"
#include "RHI/DeviceContext.h"


namespace ElecNeko
{
    class ElecNekoPipeline;

    class ElecNekoDescriptorsBase
    {
    public:
        ElecNekoDescriptorsBase() : m_vkDescriptorPool(VK_NULL_HANDLE), m_vkDescriptorSetLayout(VK_NULL_HANDLE), m_totalBindings(0), m_allocatedCount(0) {}
        virtual ~ElecNekoDescriptorsBase() = default;

        virtual bool Create(DeviceContext *device, const void *parms) = 0;
        virtual void Cleanup(DeviceContext *device);

    public:
        static constexpr int MAX_DESCRIPTOR_SETS = 256;
        VkDescriptorPool m_vkDescriptorPool;
        VkDescriptorSetLayout m_vkDescriptorSetLayout;
        VkDescriptorSet m_vkDescriptorSets[MAX_DESCRIPTOR_SETS];

    protected:
        int m_totalBindings;
        int m_allocatedCount;
    };
} // namespace ElecNeko
