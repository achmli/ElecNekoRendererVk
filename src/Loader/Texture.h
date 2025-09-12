#pragma once

#include <algorithm>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "RHI/Buffer.h"
#include "RHI/Image.h"

#include "stb_image.h"

namespace ElecNeko
{
    struct TextureProperty
    {
        int width;
        int height;
        int components;
        std::vector<uint8_t> texData;
    };

    class Texture
    {
    public:
        Texture() : width(0), height(0), components(0), isLoaded(false) {}
        Texture(std::string texName, unsigned char *data, int w, int h, int c);
        Texture(DeviceContext *device, const std::string &texName, unsigned char *data, int w, int h, int c);
        ~Texture() = default;

        bool LoadTexture(DeviceContext *device, const std::string &filename);

        [[nodiscard]] TextureProperty ExtractProperties() const
        {
            TextureProperty outProps;
            outProps.width = width;
            outProps.height = height;
            outProps.components = components;
            outProps.texData = texData;
            return outProps;
        }

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

    class TextureArray
    {
    public:
        TextureArray() : width(0), height(0), layers(0), components(0), isLoaded(false) {}
        ~TextureArray() = default;

        bool CreateFromFiles(DeviceContext *device, const std::vector<std::string> &filenames, int w, int h, int c);
        bool CreateFromData(DeviceContext *device, std::vector<TextureProperty> &properties, int w, int h, int c, const std::string &texname = "texture_array");

        void Cleanup(DeviceContext *device);

        bool ValidateAndPrepareTextures(const std::vector<std::string> &filenames, std::vector<std::vector<uint8_t>> &textureData);
        bool UploadTextureData(DeviceContext *device, const std::vector<std::vector<uint8_t>> &textureData);

    public:
        int width;
        int height;
        int layers;
        int components;
        bool isLoaded;
        std::string name;

        ArrayImage m_arrayImage;
    };
} // namespace ElecNeko
