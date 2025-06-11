#pragma once

#include <vector>

#include "Loader/Texture.h"

namespace ElecNeko
{
    class SkyBox
    {
    public:
        SkyBox() : height(0), width(0), components(0), isLoaded(false) {}
        SkyBox(const std::string &texName, unsigned char *data, int w, int h, int c) : name(texName), width(w), height(h), components(c), isLoaded(false)
        {
            size_t faceSize = w * h * c;
            texDatas.resize(6);

            for (int i = 0; i < 6; i++)
            {
                texDatas[i].resize(faceSize);

                std::copy(data + i * faceSize, data + (i + 1) * faceSize, texDatas[i].begin()); 
            }
        }

        bool LoadFromFile(DeviceContext *device, const std::string &filename);

        void Cleanup(DeviceContext *device);
    public:
        int height;
        int width;
        int components;
        bool isLoaded;
        std::vector<std::vector<uint8_t>> texDatas;
        std::string name;

        CubeImage m_cubeImage;
    };
}