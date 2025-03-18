#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>
#include "stb_image.h"

namespace ElecNeko
{
    class Texture
    {
    public:
        Texture() : width(0), height(0), components(0) {}

        Texture(std::string texName, unsigned char *data, int w, int h, int c);

        ~Texture() = default;

        bool LoadTexture(const std::string &filename);

    public:
        int width;
        int height;
        int components;
        std::vector<unsigned char> texData;
        std::string name;
    };

    inline Texture::Texture(std::string texName, unsigned char *data, int w, int h, int c) :
        name(std::move(texName)), width(w), height(h), components(c)
    {
        texData.resize(width * height * components);
        std::copy_n(data, width * height * components, texData.begin());
    }

    inline bool Texture::LoadTexture(const std::string &filename)
    {
        name = filename;
        components = 4;
        unsigned char *data = stbi_load(filename.c_str(), &width, &height, nullptr, components);

        const std::filesystem::path p(name);

        if (data == nullptr)
        {
            printf("Fail to Load %s!", p.filename().string().c_str());
        }

        texData.resize(width * height * components);

        std::copy_n(data, width * height * components, texData.begin());
        stbi_image_free(data);
        return true;
    }
} // namespace ElecNeko
