#include "SkyBox.h"

namespace ElecNeko
{
    std::vector<std::string> texNames = {"posx.jpg", "negx.jpg", "posy.jpg", "negy.jpg", "posz.jpg", "negz.jpg"};

    bool SkyBox::LoadFromFile(DeviceContext* device, const std::string& filename)
    { 
        name = "../res/CubeMaps/" + filename + "/";
        components = 4;

        VkDeviceSize imageSize = 0;
        std::vector<VkDeviceSize> picSize(6);

        texDatas.resize(6);
        for (int i = 0; i < 6; i++)
        {
            std::string path = name + texNames[i];
            stbi_set_flip_vertically_on_load(false);
            uint8_t *pixels = stbi_load(path.c_str(), &width, &height, nullptr, components);

            VkDeviceSize size = width * height * components;

            if (!pixels)
            {
                std::cerr << "Failed to load texture: " << path << std::endl;
                return false;
            }

            texDatas[i].assign(pixels, pixels + size);

            picSize[i] = imageSize;
            imageSize += size;

            stbi_image_free(pixels);
        }

        // Create Image
        {
            if (!m_cubeImage.Create(device, width, height))
            {
                std::cerr << "Failed to create Vulkan image for Cubemep " << std::endl;
                return false;
            }
        }

        // copy to a buffer
        Buffer stagingBuffer;
        stagingBuffer.Allocate(device, nullptr, imageSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        {
            uint8_t *mappedData = (uint8_t *) stagingBuffer.MapBuffer(device);
            
            for (int i = 0; i < 6; i++)
            {
                memcpy(mappedData + static_cast<uint32_t>(picSize[i]), texDatas[i].data(), texDatas[i].size());
            }

            stagingBuffer.UnmapBuffer(device);
        }

        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        m_cubeImage.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        std::vector<VkBufferImageCopy> regions(6);
        for (int i = 0; i < 6; i++)
        {
            regions[i].bufferOffset = picSize[i];
            regions[i].bufferRowLength = 0;
            regions[i].bufferImageHeight = 0;
            regions[i].imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            regions[i].imageSubresource.mipLevel = 0;
            regions[i].imageSubresource.baseArrayLayer=i;
            regions[i].imageSubresource.layerCount = 1;
            regions[i].imageOffset = {0, 0, 0};
            regions[i].imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            // vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.m_vkBuffer, m_cubeImage.m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &regions[i]);
        }

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.m_vkBuffer, m_cubeImage.m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, static_cast<uint32_t>(regions.size()), regions.data());

        m_cubeImage.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);

        stagingBuffer.Cleanup(device);

        isLoaded = true;

        return true;
    }

    void SkyBox::Cleanup(DeviceContext* device)
    {
        if (!isLoaded)
            return;

        texDatas.clear();
        m_cubeImage.Cleanup(device);
    }
}