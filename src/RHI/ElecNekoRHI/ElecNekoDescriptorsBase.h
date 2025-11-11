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

    class ElecNekoDescriptorsCreateParmsBase
    {
        virtual ~ElecNekoDescriptorsCreateParmsBase() = default;
    };

    class ElecNekoDescriptorsBase
    {
    public:
        ElecNekoDescriptorsBase() : m_vkDescriptorPool(VK_NULL_HANDLE), m_vkDescriptorSetLayout(VK_NULL_HANDLE), m_totalBindings(0), m_allocatedCount(0) {}
        virtual ~ElecNekoDescriptorsBase() = default;

        bool Create(DeviceContext *device, const ElecNekoDescriptorsCreateParmsBase &parms);
        virtual void Cleanup(DeviceContext *device)
        {
            if (m_vkDescriptorPool != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorPool(device->m_vkDevice, m_vkDescriptorPool, nullptr);
                m_vkDescriptorPool = VK_NULL_HANDLE;
            }
            if (m_vkDescriptorSetLayout != VK_NULL_HANDLE)
            {
                vkDestroyDescriptorSetLayout(device->m_vkDevice, m_vkDescriptorSetLayout, nullptr);
                m_vkDescriptorSetLayout = VK_NULL_HANDLE;
            }
            m_allocatedCount = 0;
            m_totalBindings = 0;
        }

    public:
        static constexpr int MAX_DESCRIPTOR_SETS = 256;
        VkDescriptorPool m_vkDescriptorPool;
        VkDescriptorSetLayout m_vkDescriptorSetLayout;
        VkDescriptorSet m_vkDescriptorSets[MAX_DESCRIPTOR_SETS];

    protected:
        virtual bool BuildBindingsAndPools(const ElecNekoDescriptorsCreateParmsBase &parms, std::vector<VkDescriptorSetLayoutBinding> &outLayoutBindings,
                                           std::vector<VkDescriptorPoolSize> &outPoolSizes, int &outMaxSets) = 0;

        int m_totalBindings;
        int m_allocatedCount;
    };
} // namespace ElecNeko
