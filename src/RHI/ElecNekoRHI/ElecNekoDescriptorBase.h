#pragma once

#include "ElecNekoDescriptorsBase.h"

namespace ElecNeko
{
    class ElecNekoDescriptorBase
    {
    public:
        ElecNekoDescriptorBase() : m_parent(nullptr), m_id(-1) {}
        virtual ~ElecNekoDescriptorBase() = default;

        virtual void BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNekoPipeline *pso) = 0;

    private:
        struct DescriptorBindings
        {
            int bindingPoint;
            VkDescriptorType type;
            union
            {
                VkDescriptorBufferInfo bufferInfo;
                VkDescriptorImageInfo imageInfo;
            };
            bool isImage;
        };

        ElecNekoDescriptorsBase *m_parent = nullptr;
        int m_id;

        std::vector<DescriptorBindings> m_bindings;
    };
} // namespace ElecNeko
