#include "RHI2/Vulkan/VulkanDevice.h"

#include "RHI2/Vulkan/VulkanBuffer.h"

#include "RHI2/Vulkan/VulkanTexture.h"

#include "RHI2/Vulkan/VulkanUpload.h"

#include "RHI2/Vulkan/VulkanSampler.h"

#include "RHI2/Vulkan/VulkanContext.h"

#include "RHI/DeviceContext.h"

#include <cstring>

namespace RHI
{
    VulkanDevice::VulkanDevice(DeviceContext *device) : m_context(std::make_unique<VulkanContext>(device)) {}

    VulkanDevice::~VulkanDevice() = default;

    Backend VulkanDevice::GetBackend() const { return Backend::Vulkan; }

    std::unique_ptr<Buffer> VulkanDevice::CreateBuffer(const BufferDesc &desc, const void *initialData, UploadBatch *uploadBatch)
    {
        auto buffer = std::make_unique<VulkanBuffer>(m_context.get(), desc);

        if (!buffer->Create())
        {
            return nullptr;
        }

        if (initialData != nullptr && desc.size > 0)
        {
            if (desc.cpuVisible)
            {
                void *mapped = buffer->Map();

                if (mapped == nullptr)
                {
                    return nullptr;
                }

                std::memcpy(mapped, initialData, static_cast<size_t>(desc.size));

                buffer->Unmap();
            }
            else
            {
                bool uploadOk = false;

                if (uploadBatch != nullptr)
                {
                    VulkanUploadBatch *vulkanUploadBatch = dynamic_cast<VulkanUploadBatch *>(uploadBatch);

                    if (vulkanUploadBatch == nullptr)
                    {
                        return nullptr;
                    }

                    uploadOk = vulkanUploadBatch->UploadBufferData(buffer->GetVkBuffer(), initialData, desc.size, desc.debugName);
                }
                else
                {
                    uploadOk = VulkanUpload::UploadBufferData(m_context.get(), buffer->GetVkBuffer(), initialData, desc.size, desc.debugName);
                }

                if (!uploadOk)
                {
                    return nullptr;
                }
            }
        }

        return buffer;
    }

    static bool UploadInitialTextureData(VulkanContext *context, VulkanTexture *texture, const TextureDesc &desc, const void *initialData,
                                         uint64_t initialDataSize)
    {
        if (context == nullptr || texture == nullptr || initialData == nullptr || initialDataSize == 0)
        {
            return false;
        }

        const bool uploadOk =
                VulkanUpload::UploadTexture2DArrayData(context, texture->GetVkImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                       desc.width, desc.height, desc.layers, initialData, initialDataSize, desc.debugName);

        if (!uploadOk)
        {
            return false;
        }

        texture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        return true;
    }

    static bool InitializeEmptyTextureLayout(DeviceContext *device, VulkanTexture *texture)
    {
        if (device == nullptr || texture == nullptr)
        {
            return false;
        }

        VkCommandBuffer commandBuffer = device->BeginSingleTimeCommands();

        if (commandBuffer == VK_NULL_HANDLE)
        {
            return false;
        }

        texture->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        device->EndSingleTimeCommands(commandBuffer);

        return true;
    }

    std::unique_ptr<Texture> VulkanDevice::CreateTexture(const TextureDesc &desc, const void *initialData, uint64_t initialDataSize, UploadBatch *uploadBatch)
    {
        TextureDesc actualDesc = desc;

        if (initialData != nullptr && initialDataSize > 0)
        {
            actualDesc.usage = actualDesc.usage | RHI::TextureUsage::TransferDst;
        }

        auto texture = std::make_unique<VulkanTexture>(m_context.get(), actualDesc);

        if (!texture->Create2DArray())
        {
            return nullptr;
        }

        if (initialData != nullptr && initialDataSize > 0)
        {
            bool uploadOk = false;

            if (uploadBatch != nullptr)
            {
                VulkanUploadBatch *vulkanUploadBatch = dynamic_cast<VulkanUploadBatch *>(uploadBatch);

                if (vulkanUploadBatch == nullptr)
                {
                    return nullptr;
                }

                uploadOk =
                        vulkanUploadBatch->UploadTexture2DArrayData(texture->GetVkImage(), VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
                                                                    desc.width, desc.height, desc.layers, initialData, initialDataSize, desc.debugName);
            }
            else
            {
                uploadOk = VulkanUpload::UploadTexture2DArrayData(m_context.get(), texture->GetVkImage(), VK_IMAGE_LAYOUT_UNDEFINED,
                                                                  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, desc.width, desc.height, desc.layers, initialData,
                                                                  initialDataSize, desc.debugName);
            }

            if (!uploadOk)
            {
                return nullptr;
            }

            texture->SetCurrentLayout(VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);
        }
        else
        {
            VkCommandBuffer commandBuffer = m_context->BeginSingleTimeCommands();

            texture->TransitionImageLayout(commandBuffer, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

            m_context->EndSingleTimeCommands(commandBuffer);
        }

        return texture;
    }

    std::unique_ptr<Sampler> VulkanDevice::CreateSampler(const SamplerDesc &desc)
    {
        auto sampler = std::make_unique<VulkanSampler>(m_context.get(), desc);

        if (!sampler->Create())
        {
            return nullptr;
        }

        return sampler;
    }

    std::unique_ptr<UploadBatch> VulkanDevice::CreateUploadBatch() { return std::make_unique<VulkanUploadBatch>(m_context.get()); }

    void VulkanDevice::WaitIdle()
    {
        if (m_context->GetVkDevice() != nullptr)
        {
            vkDeviceWaitIdle(m_context->GetVkDevice());
        }
    }

    void VulkanDevice::FlushDeferredDeletes()
    {
        if (m_context)
        {
            m_context->FlushDeferredDeletes();
        }
    }
} // namespace RHI
