//
// Created by ElecNekoSurface on 25-9-10.
//
#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "Texture.h"

#include "stb_image_resize.h"


namespace ElecNeko
{
    Texture::Texture(std::string texName, unsigned char *data, int w, int h, int c) : name(std::move(texName)), width(w), height(h), components(c)
    {
        texData.resize(w * h * c);
        std::copy_n(data, w * h * c, texData.begin());
    }

    Texture::Texture(DeviceContext *device, const std::string &texName, unsigned char *data, int w, int h, int c) :
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

    bool Texture::LoadTexture(DeviceContext *device, const std::string &filename)
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

    void Texture::Cleanup(DeviceContext *device)
    {
        if (!isLoaded)
            return;

        m_image.Cleanup(device);

        texData.clear();
    }

    bool TextureArray::CreateFromFiles(DeviceContext *device, const std::vector<std::string> &filenames, int w, int h, int c)
    {
        if (filenames.empty())
        {
            std::cerr << "TextureArray::CreateFromFiles: No filenames provided.\n";
            return false;
        }

        name = "TextureArray_" + std::to_string(filenames.size()) + "_layers";
        layers = static_cast<int>(filenames.size());
        width = w;
        height = h;
        components = c;

        std::vector<std::vector<uint8_t>> textureData;
        if (!ValidateAndPrepareTextures(filenames, textureData))
        {
            std::cerr << "TextureArray::CreateFromFiles: Failed to validate and prepare textures.\n";
            return false;
        }

        return UploadTextureData(device, textureData);
    }

    bool TextureArray::CreateFromData(DeviceContext *device, std::vector<TextureProperty> &properties, int w, int h, int c, const std::string &texname)
    {
        if (properties.empty())
        {
            std::cerr << "TextureArray::CreateFromData: No texture properties provided.\n";
            return false;
        }

        name = texname.empty() ? "TextureArray_" + std::to_string(properties.size()) + "_layers" : texname;
        layers = static_cast<int>(properties.size());
        width = w;
        height = h;
        components = c;

        std::vector<std::vector<uint8_t>> textureData;
        textureData.reserve(properties.size());
        for (int i = 0; i < properties.size(); i++)
        {
            std::vector<uint8_t> data;
            data.reserve(width * height * components);
            if (properties[i].width != width || properties[i].height != height || properties[i].components != components)
            {
                std::vector<uint8_t> resizedData(width * height * components);
                if (stbir_resize_uint8(properties[i].texData.data(), properties[i].width, properties[i].height, 0, resizedData.data(), width, height, 0,
                                       components) == 0)
                {
                    std::cerr << "TextureArray::CreateFromData: Failed to resize texture at index " << i << ".\n";
                    return false;
                }
                data = std::move(resizedData);
            }
            else
            {
                data = properties[i].texData;
            }
            textureData.push_back(std::move(data));
        }

        return UploadTextureData(device, textureData);
    }

    void TextureArray::Cleanup(DeviceContext *device)
    {
        if (isLoaded)
            m_arrayImage.Cleanup(device);

        // for (auto &data: m_texData)
        // {
        //     data.clear();
        // }
        // m_texData.clear();
    }

    bool TextureArray::ValidateAndPrepareTextures(const std::vector<std::string> &filenames, std::vector<std::vector<uint8_t>> &textureData)
    {
        textureData.clear();
        textureData.reserve(filenames.size());

        for (auto &filename: filenames)
        {
            int w, h, c;
            uint8_t *pixels = stbi_load(filename.c_str(), &w, &h, &c, components);
            if (!pixels)
            {
                std::cerr << "TextureArray::ValidateAndPrepareTextures: Failed to load texture: " << filename << "\n";
                return false;
            }
            if (w != width || h != height)
            {
                std::vector<uint8_t> resizedPixels(width * height * components);
                if (stbir_resize_uint8(pixels, w, h, 0, resizedPixels.data(), width, height, 0, components) == 0)
                {
                    std::cerr << "TextureArray::ValidateAndPrepareTextures: Failed to resize texture: " << filename << "\n";
                    stbi_image_free(pixels);
                    return false;
                }
                textureData.push_back(std::move(resizedPixels));
            }
            else
            {
                std::vector<uint8_t> data;
                data.assign(pixels, pixels + width * height * components);
                textureData.push_back(std::move(data));
            }

            stbi_image_free(pixels);
        }

        return true;
    }

    bool TextureArray::UploadTextureData(DeviceContext *device, const std::vector<std::vector<uint8_t>> &textureData)
    {
        ArrayImage::CreateParms_t parms{};
        parms.usageFlags = VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT;
        parms.format = VK_FORMAT_R8G8B8A8_UNORM;
        parms.width = width;
        parms.height = height;
        parms.arrayLayers = layers;

        if (!m_arrayImage.Create(device, parms))
        {
            std::cerr << "TextureArray::UploadTextureData: Failed to create Vulkan image for texture array: " << name << "\n";
            return false;
        }

        VkDeviceSize totalSize = width * height * components * layers;
        std::vector<uint8_t> combinedData;
        combinedData.reserve(totalSize);
        for (const auto &data: textureData)
        {
            combinedData.insert(combinedData.end(), data.begin(), data.end());
        }

        Buffer stagingBuffer;
        if (!stagingBuffer.Allocate(device, combinedData.data(), static_cast<int>(totalSize), VK_BUFFER_USAGE_TRANSFER_SRC_BIT))
        {
            std::cerr << "TextureArray::UploadTextureData: Failed to allocate staging buffer for texture array: " << name << "\n";
            m_arrayImage.Cleanup(device);
            return false;
        }

        VkCommandBuffer cmdBuffer = device->CreateCommandBuffer(VK_COMMAND_BUFFER_LEVEL_PRIMARY);

        m_arrayImage.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

        std::vector<VkBufferImageCopy> regions;
        regions.reserve(layers);

        VkDeviceSize layerSize = width * height * components;

        for (int layer = 0; layer < layers; layer++)
        {
            VkBufferImageCopy region = {};
            region.bufferOffset = layerSize * layer;
            region.bufferRowLength = 0;
            region.bufferImageHeight = 0;
            region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            region.imageSubresource.mipLevel = 0;
            region.imageSubresource.baseArrayLayer = layer;
            region.imageSubresource.layerCount = 1;
            region.imageOffset = {0, 0, 0};
            region.imageExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height), 1};
            regions.push_back(region);
        }

        vkCmdCopyBufferToImage(cmdBuffer, stagingBuffer.m_vkBuffer, m_arrayImage.m_vkImage, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                               static_cast<uint32_t>(regions.size()), regions.data());

        m_arrayImage.TransitionLayout(cmdBuffer, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

        device->FlushCommandBuffer(cmdBuffer, device->m_vkGraphicsQueue);

        stagingBuffer.Cleanup(device);

        isLoaded = true;
        return true;
    }
} // namespace ElecNeko
