//
//  Samplers.h
//
#pragma once
#include <vulkan/vulkan.hpp>
#include "DeviceContext.h"

/*
====================================================
Samplers
====================================================
*/
class Samplers
{
public:
    static bool InitializeSamplers(DeviceContext *device);
    static void Cleanup(DeviceContext *device);

    static VkSampler m_samplerStandard;
    static VkSampler m_samplerDepth;
};

namespace ElecNeko
{
    class ElecNekoSampler
    {
    public:
        static bool InitializeSampler(DeviceContext *device);
        static bool InitializeIBLSampler(DeviceContext *device, float maxLod);
        static void Cleanup(DeviceContext *device);

        static VkSampler m_samplerTexture;
        static VkSampler m_samplerCubemap;
        static VkSampler m_samplerShadow;
        static VkSampler m_samplerIBL;
    };
} // namespace ElecNeko
