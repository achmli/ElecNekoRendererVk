#pragma once

#include <utility>
#include <vector>
#include <algorithm>
#include <iostream>
#include <string>

#include "RHI/Image.h"
#include "RHI/Buffer.h"

#include "stb_image.h"

namespace ElecNeko
{
    class Texture
    {
    public:
        Texture() : width(0), height(0), components(0) {}
        Texture(const std::string &texName, unsigned char *data, int w, int h, int c);
        ~Texture() = default;

        bool LoadTexture(DeviceContext *device, const std::string &filename);

    public:
        int height;
        int width;
        int components;
        std::vector<uint8_t> texData;
        std::string name;

        Image m_image;
    };

    inline Texture::Texture(const std::string &texName, unsigned char *data, int w, int h, int c) : name(std::move(texName)), width(w), height(h), components(c)
    {
        texData.resize(w * h * c);
        std::copy_n(data, w * h * c, texData.begin());
    }

    inline bool Texture::LoadTexture(DeviceContext * device, const std::string &filename)
    {
        name = filename;
        components = 4;
        uint8_t *pixels = stbi_load(filename.c_str(), &width, &height, nullptr, components);

        VkDeviceSize imageSize = width * height * components;

        if (!pixels)
        {
            std::cerr << "Failed to load texture: " << name << std::endl;
            return false;
        }

        texData.assign(pixels, pixels + imageSize);

        {
            Image::CreateParms_t parms{};
            parms.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            parms.format = VK_FORMAT_R8G8B8A8_UNORM;
            parms.width = width;
            parms.height = height;
            parms.depth = 1;

            if (!m_image.Create(device, parms))
            {
                std::cerr << "Failed to create Vulkan image for texture: " << name << std::endl;
                stbi_image_free(pixels);
                return false;
            }
        }

        Buffer stagingBuffer;
        stagingBuffer.Allocate(device, texData.data(), static_cast<int>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        m_image.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        {
            VkBufferImageCopy region = {};
            region.bufferOffset = 0;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = 0;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};

            vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.m_vkBuffer, m_image.m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);
        }

        m_image.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);

        stagingBuffer.Cleanup(device);
        stbi_image_free(pixels);

        return true;
    }
}
