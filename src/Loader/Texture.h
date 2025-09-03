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
        Texture() : width(0), height(0), components(0), isLoaded(false) {}
        Texture(std::string texName, unsigned char *data, int w, int h, int c);
        Texture(DeviceContext *device, const std::string &texName, unsigned char *data, int w, int h, int c);
        ~Texture() = default;

        bool LoadTexture(DeviceContext *device, const std::string &filename);
        void DefaultTexture(DeviceContext* device, float r, float g, float b, float a);

        void Cleanup(DeviceContext *device);

    public:
        int height;
        int width;
        int components;
        int originComponents;
        bool isLoaded;
        std::vector<uint8_t> texData;
        std::string name;

        Image m_image;
    };

    inline Texture::Texture(std::string texName, unsigned char *data, int w, int h, int c) : name(std::move(texName)), width(w), height(h), components(c)
    {
        texData.resize(w * h * c);
        std::copy_n(data, w * h * c, texData.begin());
    }

    inline Texture::Texture(DeviceContext* device, const std::string& texName, unsigned char* data, int w, int h, int c) :
        name(texName), width(w), height(h), components(c)
    {
        VkDeviceSize imageSize = width * height * components;

        originComponents = c;
        texData.resize(imageSize);
        std::copy_n(data, width * height * components, texData.begin());

        {
            Image::CreateParms_t parms{};
            parms.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
            parms.format = VK_FORMAT_R8G8B8A8_UNORM;
            parms.width = width;
            parms.height = height;
            parms.depth = 1;

            m_image.Create(device, parms);
        }

        Buffer stagingBuffer;
        stagingBuffer.Allocate(device, texData.data(), static_cast<int>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT);

        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        m_image.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        {
            VkBufferImageCopy region{};
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
        
        isLoaded = true;
    }

    inline bool Texture::LoadTexture(DeviceContext * device, const std::string &filename)
    {
        name = filename;

        int origComponents = 0;
        const int reqComponents = 4;
        uint8_t *pixels = stbi_load(filename.c_str(), &width, &height, &origComponents, reqComponents);

        if (!pixels)
        {
            std::cerr << "Failed to load texture: " << name << std::endl;
            stbi_image_free(pixels);
            return false;
        }

        components = 4;
        originComponents = origComponents;

        VkDeviceSize imageSize = width * height * reqComponents;
        if (imageSize == 0)
        {
            stbi_image_free(pixels);
            return false;
        }
        if (imageSize > static_cast<size_t>(std::numeric_limits<int>::max()))
        {
            std::cerr << "Texture too large: " << name << "\n";
            stbi_image_free(pixels);
            return false;
        }

        texData.assign(pixels, pixels + imageSize);
        stbi_image_free(pixels);

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
        if (!stagingBuffer.Allocate(device, texData.data(), static_cast<int>(imageSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
        {
            std::cerr << "Failed to allocate staging buffer for texture: " << name << "\n";
            m_image.Cleanup(device);
            texData.clear();
            return false;
        }

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
        

        isLoaded = true;
        return true;
    }

    inline void Texture::Cleanup(DeviceContext *device)
    {
        if (!isLoaded)
            return;

        m_image.Cleanup(device);

        texData.clear();
    }
}
