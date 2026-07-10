//
//  Descriptor.cpp
//
#include "Descriptor.h"
#include <assert.h>
#include <vector>
#include "DeviceContext.h"
#include "Pipeline.h"

// #include "RHI2/RHIBuffer.h"
// #include "RHI2/RHISampler.h"
// #include "RHI2/RHITexture.h"
#include "RHI2/RHIBinding.h"
#include "RHI2/Vulkan/VulkanBuffer.h"
#include "RHI2/Vulkan/VulkanSampler.h"
#include "RHI2/Vulkan/VulkanTexture.h"


/*
========================================================================================================

Descriptors

========================================================================================================
*/

/*
====================================================
Descriptors::Create
====================================================
*/
bool Descriptors::Create(DeviceContext *device, const CreateParms_t &parms)
{
    VkResult result;

    m_parms = parms;

    //
    //	Create the non-global pool
    //
    std::vector<VkDescriptorPoolSize> poolSizes;
    const int numUniforms = parms.numUniformsFragment + parms.numUniformsVertex;
    if (numUniforms > 0)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = numUniforms * MAX_DESCRIPTOR_SETS;
        poolSizes.push_back(poolSize);
    }
    if (parms.numImageSamplers > 0)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = parms.numImageSamplers * MAX_DESCRIPTOR_SETS;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t) poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_DESCRIPTOR_SETS;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    result = vkCreateDescriptorPool(device->m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create descriptor pool\n");
        assert(0);
        return false;
    }

    //
    // Create Descriptor Set Layout
    //
    VkDescriptorSetLayoutBinding *uniformBindings = (VkDescriptorSetLayoutBinding *) alloca(sizeof(VkDescriptorSetLayoutBinding) * (numUniforms));
    memset(uniformBindings, 0, sizeof(VkDescriptorSetLayoutBinding) * numUniforms);

    int idx = 0;

    const int numVertexUniforms = parms.numUniformsVertex;
    for (int i = 0; i < parms.numUniformsVertex; i++)
    {
        uniformBindings[idx].binding = idx;
        uniformBindings[idx].descriptorCount = 1;
        uniformBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBindings[idx].pImmutableSamplers = nullptr;
        uniformBindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        idx++;
    }

    for (int i = 0; i < parms.numUniformsFragment; i++)
    {
        uniformBindings[idx].binding = idx;
        uniformBindings[idx].descriptorCount = 1;
        uniformBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uniformBindings[idx].pImmutableSamplers = nullptr;
        uniformBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        idx++;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = numUniforms;
    layoutInfo.pBindings = uniformBindings;

    result = vkCreateDescriptorSetLayout(device->m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create descriptor set layout\n");
        assert(0);
        return false;
    }

    //
    //	Create Descriptor Sets
    //
    VkDescriptorSetLayout layouts[MAX_DESCRIPTOR_SETS];
    for (int i = 0; i < MAX_DESCRIPTOR_SETS; i++)
    {
        layouts[i] = m_vkDescriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_vkDescriptorPool;
    allocInfo.descriptorSetCount = MAX_DESCRIPTOR_SETS;
    allocInfo.pSetLayouts = layouts;

    result = vkAllocateDescriptorSets(device->m_vkDevice, &allocInfo, m_vkDescriptorSets);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to allocate descriptor set\n");
        assert(0);
        return false;
    }

    return true;
}

/*
====================================================
Descriptors::Create
====================================================
*/
void Descriptors::Cleanup(DeviceContext *device)
{
    // Free the descriptor sets
    vkFreeDescriptorSets(device->m_vkDevice, m_vkDescriptorPool, MAX_DESCRIPTOR_SETS, m_vkDescriptorSets);

    // Destroy descriptor set layout
    vkDestroyDescriptorSetLayout(device->m_vkDevice, m_vkDescriptorSetLayout, nullptr);

    // Destroy Descriptor Pool
    vkDestroyDescriptorPool(device->m_vkDevice, m_vkDescriptorPool, nullptr);
}


/*
========================================================================================================

Descriptor

========================================================================================================
*/

/*
====================================================
Descriptor::Descriptor
====================================================
*/
Descriptor::Descriptor()
{
    m_parent = NULL;
    m_id = -1;
    m_numImages = 0;
    m_numBuffers = 0;
    memset(m_imageInfo, 0, sizeof(VkDescriptorImageInfo) * MAX_IMAGEINFO);
    memset(m_bufferInfo, 0, sizeof(VkDescriptorBufferInfo) * MAX_BUFFERS);
}

/*
====================================================
Descriptor::BindImage
====================================================
*/
void Descriptor::BindImage(VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler, int slot)
{
    assert(slot < MAX_IMAGEINFO);
    assert(m_numImages < MAX_IMAGEINFO);
    m_imageInfo[slot].imageLayout = imageLayout;
    m_imageInfo[slot].imageView = imageView;
    m_imageInfo[slot].sampler = sampler;
    m_numImages++;
}

/*
====================================================
Descriptor::BindBuffer
====================================================
*/
void Descriptor::BindBuffer(Buffer *uniformBuffer, int offset, int size, int slot)
{
    assert(slot < MAX_BUFFERS);
    assert(m_numBuffers < MAX_BUFFERS);
    m_bufferInfo[slot].buffer = uniformBuffer->m_vkBuffer;
    m_bufferInfo[slot].offset = offset;
    m_bufferInfo[slot].range = size;
    m_numBuffers++;
}

/*
====================================================
Descriptor::BindDescriptor
====================================================
*/
void Descriptor::BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, Pipeline *pso)
{
    const int numDescriptors = m_numImages + m_numBuffers;
    const int allocationSize = sizeof(VkWriteDescriptorSet) * numDescriptors;
    VkWriteDescriptorSet *descriptorWrites = (VkWriteDescriptorSet *) alloca(allocationSize);
    memset(descriptorWrites, 0, allocationSize);

    int idx = 0;
    for (int i = 0; i < m_numBuffers; i++)
    {
        descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[idx].dstSet = m_parent->m_vkDescriptorSets[m_id];
        descriptorWrites[idx].dstBinding = idx;
        descriptorWrites[idx].dstArrayElement = 0;
        descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[idx].descriptorCount = 1;
        descriptorWrites[idx].pBufferInfo = &m_bufferInfo[i];

        idx++;
    }

    for (int i = 0; i < m_numImages; i++)
    {
        descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[idx].dstSet = m_parent->m_vkDescriptorSets[m_id];
        descriptorWrites[idx].dstBinding = idx;
        descriptorWrites[idx].dstArrayElement = 0;
        descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[idx].descriptorCount = 1;
        descriptorWrites[idx].pImageInfo = &m_imageInfo[i];

        idx++;
    }

    vkUpdateDescriptorSets(device->m_vkDevice, (uint32_t) numDescriptors, descriptorWrites, 0, nullptr);
    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[m_id], 0, nullptr);
}

void Descriptor::BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNeko::ElecNekoPipeline *pso)
{
    const int numDescriptors = m_numImages + m_numBuffers;
    const int allocationSize = sizeof(VkWriteDescriptorSet) * numDescriptors;
    VkWriteDescriptorSet *descriptorWrites = (VkWriteDescriptorSet *) alloca(allocationSize);
    memset(descriptorWrites, 0, allocationSize);

    int idx = 0;
    for (int i = 0; i < m_numBuffers; i++)
    {
        descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[idx].dstSet = m_parent->m_vkDescriptorSets[m_id];
        descriptorWrites[idx].dstBinding = idx;
        descriptorWrites[idx].dstArrayElement = 0;
        descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[idx].descriptorCount = 1;
        descriptorWrites[idx].pBufferInfo = &m_bufferInfo[i];

        idx++;
    }

    for (int i = 0; i < m_numImages; i++)
    {
        descriptorWrites[idx].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[idx].dstSet = m_parent->m_vkDescriptorSets[m_id];
        descriptorWrites[idx].dstBinding = idx;
        descriptorWrites[idx].dstArrayElement = 0;
        descriptorWrites[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        descriptorWrites[idx].descriptorCount = 1;
        descriptorWrites[idx].pImageInfo = &m_imageInfo[i];

        idx++;
    }

    vkUpdateDescriptorSets(device->m_vkDevice, (uint32_t) numDescriptors, descriptorWrites, 0, nullptr);
    vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[m_id], 0, nullptr);
}


bool Descriptors::ElecNekoCreate(DeviceContext *device, const CreateParms_t &parms)
{
    VkResult result;

    m_parms = parms;

    //
    //	Create the non-global pool
    //
    std::vector<VkDescriptorPoolSize> poolSizes;
    const int numUniforms = parms.numUniformsFragment + parms.numUniformsVertex + parms.numImageSamplers;
    if (numUniforms - parms.numImageSamplers > 0)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSize.descriptorCount = numUniforms * MAX_DESCRIPTOR_SETS;
        poolSizes.push_back(poolSize);
    }
    if (parms.numImageSamplers > 0)
    {
        VkDescriptorPoolSize poolSize;
        poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSize.descriptorCount = parms.numImageSamplers * MAX_DESCRIPTOR_SETS;
        poolSizes.push_back(poolSize);
    }

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (uint32_t) poolSizes.size();
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = MAX_DESCRIPTOR_SETS;
    poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

    result = vkCreateDescriptorPool(device->m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create descriptor pool\n");
        assert(0);
        return false;
    }

    //
    // Create Descriptor Set Layout
    //
    VkDescriptorSetLayoutBinding *uniformBindings = (VkDescriptorSetLayoutBinding *) alloca(sizeof(VkDescriptorSetLayoutBinding) * (numUniforms));
    memset(uniformBindings, 0, sizeof(VkDescriptorSetLayoutBinding) * numUniforms);

    int idx = 0;

    for (int i = 0; i < parms.numUniformsVertex; i++)
    {
        uniformBindings[idx].binding = idx;
        uniformBindings[idx].descriptorCount = 1;
        uniformBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBindings[idx].pImmutableSamplers = nullptr;
        uniformBindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        idx++;
    }

    for (int i = 0; i < parms.numUniformsFragment; i++)
    {
        uniformBindings[idx].binding = idx;
        uniformBindings[idx].descriptorCount = 1;
        uniformBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uniformBindings[idx].pImmutableSamplers = nullptr;
        uniformBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        idx++;
    }

    for (int i = 0; i < parms.numImageSamplers; i++)
    {
        uniformBindings[idx].binding = idx;
        uniformBindings[idx].descriptorCount = 1;
        uniformBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        uniformBindings[idx].pImmutableSamplers = nullptr;
        uniformBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        idx++;
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo = {};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = numUniforms;
    layoutInfo.pBindings = uniformBindings;

    result = vkCreateDescriptorSetLayout(device->m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create descriptor set layout\n");
        assert(0);
        return false;
    }

    //
    //	Create Descriptor Sets
    //
    VkDescriptorSetLayout layouts[MAX_DESCRIPTOR_SETS];
    for (int i = 0; i < MAX_DESCRIPTOR_SETS; i++)
    {
        layouts[i] = m_vkDescriptorSetLayout;
    }
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = m_vkDescriptorPool;
    allocInfo.descriptorSetCount = MAX_DESCRIPTOR_SETS;
    allocInfo.pSetLayouts = layouts;

    result = vkAllocateDescriptorSets(device->m_vkDevice, &allocInfo, m_vkDescriptorSets);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to allocate descriptor set\n");
        assert(0);
        return false;
    }

    return true;
}

namespace ElecNeko
{
    bool ElecNekoDescriptors::Create(DeviceContext *device, const CreateParms_t &parms)
    {
        VkResult result;
        m_parms = parms;

        std::vector<VkDescriptorPoolSize> poolSizes;

        int numUniforms = parms.numUniformsVertex + parms.numUniformsFragment;
        if (numUniforms > 0)
        {
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            poolSize.descriptorCount = numUniforms * MAX_DESCRIPTOR_SETS;
            poolSizes.push_back(poolSize);
        }

        int numStorages = parms.numStorageVertex + parms.numStorageFragment;
        if (numStorages > 0)
        {
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            poolSize.descriptorCount = numStorages * MAX_DESCRIPTOR_SETS;
            poolSizes.push_back(poolSize);
        }

        if (parms.numImageSamplers > 0)
        {
            VkDescriptorPoolSize poolSize;
            poolSize.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            poolSize.descriptorCount = parms.numImageSamplers * MAX_DESCRIPTOR_SETS;
            poolSizes.push_back(poolSize);
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = MAX_DESCRIPTOR_SETS;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        result = vkCreateDescriptorPool(device->m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool);
        if (VK_SUCCESS != result)
        {
            printf("ERROR: Failed to create descriptor pool\n");
            assert(0);
            return false;
        }

        int totalBindings = numUniforms + numStorages + parms.numImageSamplers;
        std::vector<VkDescriptorSetLayoutBinding> layoutBindings(totalBindings);
        int idx = 0;

        for (int i = 0; i < parms.numUniformsVertex; i++)
        {
            layoutBindings[idx].binding = idx;
            layoutBindings[idx].descriptorCount = 1;
            layoutBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBindings[idx].pImmutableSamplers = nullptr;
            layoutBindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            idx++;
        }

        for (int i = 0; i < parms.numStorageVertex; i++)
        {
            layoutBindings[idx].binding = idx;
            layoutBindings[idx].descriptorCount = 1;
            layoutBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindings[idx].pImmutableSamplers = nullptr;
            layoutBindings[idx].stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
            idx++;
        }


        for (int i = 0; i < parms.numUniformsFragment; i++)
        {
            layoutBindings[idx].binding = idx;
            layoutBindings[idx].descriptorCount = 1;
            layoutBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            layoutBindings[idx].pImmutableSamplers = nullptr;
            layoutBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            idx++;
        }


        for (int i = 0; i < parms.numStorageFragment; i++)
        {
            layoutBindings[idx].binding = idx;
            layoutBindings[idx].descriptorCount = 1;
            layoutBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            layoutBindings[idx].pImmutableSamplers = nullptr;
            layoutBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            idx++;
        }


        for (int i = 0; i < parms.numImageSamplers; i++)
        {
            layoutBindings[idx].binding = idx;
            layoutBindings[idx].descriptorCount = 1;
            layoutBindings[idx].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            layoutBindings[idx].pImmutableSamplers = nullptr;
            layoutBindings[idx].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
            idx++;
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(layoutBindings.size());
        layoutInfo.pBindings = layoutBindings.data();

        result = vkCreateDescriptorSetLayout(device->m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout);
        if (VK_SUCCESS != result)
        {
            printf("ERROR: Failed to create descriptor set layout\n");
            assert(0);
            return false;
        }


        std::vector<VkDescriptorSetLayout> layouts(MAX_DESCRIPTOR_SETS, m_vkDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = MAX_DESCRIPTOR_SETS;
        allocInfo.pSetLayouts = layouts.data();

        result = vkAllocateDescriptorSets(device->m_vkDevice, &allocInfo, m_vkDescriptorSets);
        if (VK_SUCCESS != result)
        {
            printf("ERROR: Failed to allocate descriptor set\n");
            assert(0);
            return false;
        }

        return true;
    }

    void ElecNekoDescriptors::Cleanup(DeviceContext *device)
    {
        if (m_vkDescriptorPool != VK_NULL_HANDLE)
        {
            vkFreeDescriptorSets(device->m_vkDevice, m_vkDescriptorPool, MAX_DESCRIPTOR_SETS, m_vkDescriptorSets);
        }

        if (m_vkDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device->m_vkDevice, m_vkDescriptorSetLayout, nullptr);

            m_vkDescriptorSetLayout = VK_NULL_HANDLE;
        }

        if (m_vkDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device->m_vkDevice, m_vkDescriptorPool, nullptr);

            m_vkDescriptorPool = VK_NULL_HANDLE;
        }

        m_numDescriptorUsed = 0;
    }

    void ElecNekoDescriptor::BindUniformBuffer(int bindingPoint, Buffer *buffer, int offset, int size)
    {
        DescriptorBinding binding;
        binding.bindingPoint = bindingPoint;
        binding.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        binding.bufferInfo.buffer = buffer->m_vkBuffer;
        binding.bufferInfo.offset = offset;
        binding.bufferInfo.range = size;
        binding.isImage = false;

        m_bindings.push_back(binding);
    }

    void ElecNekoDescriptor::BindUniformBuffer(int bindingPoint, RHI::Buffer *buffer, int offset, int size)
    {
        if (buffer == nullptr)
        {
            return;
        }

        RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(buffer);

        if (vulkanBuffer == nullptr)
        {
            return;
        }

        ::Buffer *legacyBuffer = vulkanBuffer->GetLegacyBufferForTransition();

        if (legacyBuffer == nullptr)
        {
            return;
        }

        BindUniformBuffer(bindingPoint, legacyBuffer, offset, size);
    }

    void ElecNekoDescriptor::BindUniformBuffer(const RHI::BufferBinding &binding)
    {
        BindUniformBuffer(static_cast<int>(binding.binding), binding.buffer, static_cast<int>(binding.offset), static_cast<int>(binding.size));
    }

    void ElecNekoDescriptor::BindStorageBuffer(int bindingPoint, Buffer *buffer, int offset, int size)
    {
        DescriptorBinding binding;
        binding.bindingPoint = bindingPoint;
        binding.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        binding.bufferInfo.buffer = buffer->m_vkBuffer;
        binding.bufferInfo.offset = offset;
        binding.bufferInfo.range = size;
        binding.isImage = false;

        m_bindings.push_back(binding);
    }

    void ElecNekoDescriptor::BindStorageBuffer(const RHI::BufferBinding &binding)
    {
        BindStorageBuffer(static_cast<int>(binding.binding), binding.buffer, static_cast<int>(binding.offset), static_cast<int>(binding.size));
    }

    // void ElecNekoDescriptor::BindStorageBuffer(uint32_t binding, RHI::Buffer *buffer, VkDeviceSize offset, VkDeviceSize range)
    // {
    //     if (buffer == nullptr)
    //     {
    //         return;
    //     }

    //     ::Buffer *legacyBuffer = buffer->GetLegacyBufferForTransition();

    //     if (legacyBuffer == nullptr)
    //     {
    //         return;
    //     }

    //     BindStorageBuffer(binding, legacyBuffer, offset, range);
    // }

    void ElecNekoDescriptor::BindStorageBuffer(int bindingPoint, RHI::Buffer *buffer, int offset, int size)
    {
        if (buffer == nullptr)
        {
            return;
        }

        RHI::VulkanBuffer *vulkanBuffer = dynamic_cast<RHI::VulkanBuffer *>(buffer);

        if (vulkanBuffer == nullptr)
        {
            return;
        }

        ::Buffer *legacyBuffer = vulkanBuffer->GetLegacyBufferForTransition();

        if (legacyBuffer == nullptr)
        {
            return;
        }

        BindStorageBuffer(bindingPoint, legacyBuffer, offset, size);
    }

    void ElecNekoDescriptor::BindImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler)
    {
        DescriptorBinding binding;
        binding.bindingPoint = bindingPoint;
        binding.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        binding.imageInfo.imageLayout = imageLayout;
        binding.imageInfo.imageView = imageView;
        binding.imageInfo.sampler = sampler;
        binding.isImage = true;

        m_bindings.push_back(binding);
    }

    void ElecNekoDescriptor::BindImage(int bindingPoint, VkImageLayout imageLayout, RHI::Texture *texture, VkSampler sampler)
    {
        if (texture == nullptr)
        {
            return;
        }

        RHI::VulkanTexture *vulkanTexture = dynamic_cast<RHI::VulkanTexture *>(texture);

        if (vulkanTexture == nullptr)
        {
            return;
        }

        const VkImageView imageView = vulkanTexture->GetVkImageView();

        if (imageView == VK_NULL_HANDLE)
        {
            return;
        }

        BindImage(bindingPoint, imageLayout, imageView, sampler);
    }

    void ElecNekoDescriptor::BindImage(int bindingPoint, VkImageLayout imageLayout, RHI::Texture *texture, RHI::Sampler *sampler)
    {
        if (texture == nullptr || sampler == nullptr)
        {
            return;
        }

        RHI::VulkanTexture *vulkanTexture = dynamic_cast<RHI::VulkanTexture *>(texture);

        if (vulkanTexture == nullptr)
        {
            return;
        }

        RHI::VulkanSampler *vulkanSampler = dynamic_cast<RHI::VulkanSampler *>(sampler);

        if (vulkanSampler == nullptr)
        {
            return;
        }

        const VkImageView imageView = vulkanTexture->GetVkImageView();

        const VkSampler vkSampler = vulkanSampler->GetVkSampler();

        if (imageView == VK_NULL_HANDLE || vkSampler == VK_NULL_HANDLE)
        {
            return;
        }

        BindImage(bindingPoint, imageLayout, imageView, vkSampler);
    }

    void ElecNekoDescriptor::BindSampledTexture(int bindingPoint, RHI::Texture *texture, RHI::Sampler *sampler)
    {
        BindImage(bindingPoint, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, texture, sampler);
    }

    void ElecNekoDescriptor::BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, Pipeline *pso)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(m_bindings.size());

        for (const auto &binding: m_bindings)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_parent->m_vkDescriptorSets[m_id];
            write.dstBinding = binding.bindingPoint;
            write.dstArrayElement = 0;
            write.descriptorType = binding.type;
            write.descriptorCount = 1;

            if (binding.isImage)
            {
                write.pImageInfo = &binding.imageInfo;
            }
            else
            {
                write.pBufferInfo = &binding.bufferInfo;
            }

            descriptorWrites.push_back(write);
        }

        vkUpdateDescriptorSets(device->m_vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[m_id], 0,
                                nullptr);
    }

    void ElecNekoDescriptor::BindSampledTexture(const RHI::SampledTextureBinding &binding)
    {
        BindSampledTexture(static_cast<int>(binding.binding), binding.texture, binding.sampler);
    }

    void ElecNekoDescriptor::BindBindingSet(const RHI::BindingSetDesc &desc)
    {
        for (const RHI::BufferBinding &binding: desc.uniformBuffers)
        {
            BindUniformBuffer(binding);
        }

        for (const RHI::BufferBinding &binding: desc.storageBuffers)
        {
            BindStorageBuffer(binding);
        }

        for (const RHI::SampledTextureBinding &binding: desc.sampledTextures)
        {
            BindSampledTexture(binding);
        }
    }

    void ElecNekoDescriptor::BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNekoPipeline *pso)
    {
        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(m_bindings.size());

        for (const auto &binding: m_bindings)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_parent->m_vkDescriptorSets[m_id];
            write.dstBinding = binding.bindingPoint;
            write.dstArrayElement = 0;
            write.descriptorType = binding.type;
            write.descriptorCount = 1;

            if (binding.isImage)
            {
                write.pImageInfo = &binding.imageInfo;
            }
            else
            {
                write.pBufferInfo = &binding.bufferInfo;
            }

            descriptorWrites.push_back(write);
        }

        vkUpdateDescriptorSets(device->m_vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[m_id], 0,
                                nullptr);
    }

    bool ElecNekoDescriptorsCompute::Create(DeviceContext *device, const CreateParms_t &parms)
    {
        m_parms = parms;
        VkResult result;

        // Descriptor Pool
        std::vector<VkDescriptorPoolSize> poolSizes;
        if (parms.numUniforms > 0)
        {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(parms.numUniforms * parms.maxSets)});
        }
        if (parms.numStorageBuffers > 0)
        {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, static_cast<uint32_t>(parms.numStorageBuffers * parms.maxSets)});
        }
        if (parms.numStorageImages > 0)
        {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_STORAGE_IMAGE, static_cast<uint32_t>(parms.numStorageImages * parms.maxSets)});
        }
        if (parms.numSampledImages > 0)
        {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<uint32_t>(parms.numSampledImages * parms.maxSets)});
        }

        if (poolSizes.empty())
        {
            poolSizes.push_back({VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<uint32_t>(parms.maxSets)});
        }

        VkDescriptorPoolCreateInfo poolInfo = {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = parms.maxSets;
        poolInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

        result = vkCreateDescriptorPool(device->m_vkDevice, &poolInfo, nullptr, &m_vkDescriptorPool);
        if (result != VK_SUCCESS)
        {
            printf("ERROR: Failed to create descriptor pool\n");
            return false;
        }

        // descriptor set layout
        std::vector<VkDescriptorSetLayoutBinding> bindings;
        int idx = 0;
        for (int i = 0; i < parms.numUniforms; i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = idx++;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);
        }
        for (int i = 0; i < parms.numStorageBuffers; i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = idx++;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);
        }
        for (int i = 0; i < parms.numStorageImages; i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = idx++;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);
        }
        for (int i = 0; i < parms.numSampledImages; i++)
        {
            VkDescriptorSetLayoutBinding binding = {};
            binding.binding = idx++;
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            binding.descriptorCount = 1;
            binding.stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;
            binding.pImmutableSamplers = nullptr;
            bindings.push_back(binding);
        }

        VkDescriptorSetLayoutCreateInfo layoutInfo = {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.empty() ? nullptr : bindings.data();

        result = vkCreateDescriptorSetLayout(device->m_vkDevice, &layoutInfo, nullptr, &m_vkDescriptorSetLayout);
        if (result != VK_SUCCESS)
        {
            printf("ERROR: Failed to create descriptor set layout\n");
            return false;
        }

        // allocate sets
        std::vector<VkDescriptorSetLayout> layouts(parms.maxSets, m_vkDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo = {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_vkDescriptorPool;
        allocInfo.descriptorSetCount = parms.maxSets;
        allocInfo.pSetLayouts = layouts.data();

        result = vkAllocateDescriptorSets(device->m_vkDevice, &allocInfo, m_vkDescriptorSets);
        if (result != VK_SUCCESS)
        {
            printf("ERROR: Failed to allocate descriptor sets\n");
            vkDestroyDescriptorSetLayout(device->m_vkDevice, m_vkDescriptorSetLayout, nullptr);
            vkDestroyDescriptorPool(device->m_vkDevice, m_vkDescriptorPool, nullptr);
            m_vkDescriptorSetLayout = VK_NULL_HANDLE;
            m_vkDescriptorPool = VK_NULL_HANDLE;
            return false;
        }

        m_totalBindings = idx;
        m_allocatedCount = 0;
        return true;
    }

    void ElecNekoDescriptorsCompute::Cleanup(DeviceContext *device)
    {
        if (m_vkDescriptorSetLayout != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorSetLayout(device->m_vkDevice, m_vkDescriptorSetLayout, nullptr);
            m_vkDescriptorSetLayout = VK_NULL_HANDLE;
        }
        if (m_vkDescriptorPool != VK_NULL_HANDLE)
        {
            vkDestroyDescriptorPool(device->m_vkDevice, m_vkDescriptorPool, nullptr);
            m_vkDescriptorPool = VK_NULL_HANDLE;
        }
        m_allocatedCount = 0;
    }

    ElecNekoDescriptorCompute ElecNekoDescriptorsCompute::GetFreeDescriptor()
    {
        ElecNekoDescriptorCompute descriptor;
        descriptor.m_parent = this;
        descriptor.m_id = m_allocatedCount % m_parms.maxSets;
        m_allocatedCount++;
        return descriptor;
    }

    void ElecNekoDescriptorCompute::BindingUniform(int bindingPoint, Buffer *buffer, int offset, int size)
    {
        DescriptorBindings b{};
        b.bindingPoint = bindingPoint;
        b.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        b.bufferInfo.buffer = buffer->m_vkBuffer;
        b.bufferInfo.offset = offset;
        b.bufferInfo.range = size;
        b.isImage = false;
        m_bindings.push_back(b);
    }

    void ElecNekoDescriptorCompute::BindingStorageBuffer(int bindingPoint, Buffer *buffer, int offset, int size)
    {
        DescriptorBindings b{};
        b.bindingPoint = bindingPoint;
        b.type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        b.bufferInfo.buffer = buffer->m_vkBuffer;
        b.bufferInfo.offset = offset;
        b.bufferInfo.range = size;
        b.isImage = false;
        m_bindings.push_back(b);
    }

    void ElecNekoDescriptorCompute::BindingStorageImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler)
    {
        DescriptorBindings b{};
        b.bindingPoint = bindingPoint;
        b.type = VK_DESCRIPTOR_TYPE_STORAGE_IMAGE;
        b.imageInfo.imageLayout = imageLayout;
        b.imageInfo.imageView = imageView;
        b.imageInfo.sampler = sampler;
        b.isImage = true;
        m_bindings.push_back(b);
    }

    void ElecNekoDescriptorCompute::BindingSampledImage(int bindingPoint, VkImageLayout imageLayout, VkImageView imageView, VkSampler sampler)
    {
        DescriptorBindings b{};
        b.bindingPoint = bindingPoint;
        b.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        b.imageInfo.imageLayout = imageLayout;
        b.imageInfo.imageView = imageView;
        b.imageInfo.sampler = sampler;
        b.isImage = true;
        m_bindings.push_back(b);
    }

    void ElecNekoDescriptorCompute::BindDescriptor(DeviceContext *device, VkCommandBuffer vkCommandBuffer, ElecNekoPipeline *pso)
    {
        if (!m_parent || m_id < 0)
        {
            printf("Warning: Invalid descriptor set\n");
            return;
        }

        std::vector<VkWriteDescriptorSet> descriptorWrites;
        descriptorWrites.reserve(m_bindings.size());

        for (auto &b: m_bindings)
        {
            VkWriteDescriptorSet write = {};
            write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            write.dstSet = m_parent->m_vkDescriptorSets[m_id];
            write.dstBinding = b.bindingPoint;
            write.dstArrayElement = 0;
            write.descriptorCount = 1;
            write.descriptorType = b.type;
            if (b.isImage)
            {
                write.pImageInfo = &b.imageInfo;
            }
            else
            {
                write.pBufferInfo = &b.bufferInfo;
            }

            descriptorWrites.push_back(write);
        }

        if (!descriptorWrites.empty())
        {
            vkUpdateDescriptorSets(device->m_vkDevice, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
        }

        vkCmdBindDescriptorSets(vkCommandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, pso->m_vkPipelineLayout, 0, 1, &m_parent->m_vkDescriptorSets[m_id], 0,
                                nullptr);
    }


} // namespace ElecNeko
