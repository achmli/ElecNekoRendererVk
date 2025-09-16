//
//  Image.h
//
#pragma once
#include <vulkan/vulkan.hpp>
#include "DeviceContext.h"

namespace ElecNeko
{
    struct LayoutBarrierInfo
    {
        VkPipelineStageFlags stage;
        VkAccessFlags access;
    };
} // namespace ElecNeko

/*
====================================================
Image
====================================================
*/
class Image
{
public:
    Image() {}
    ~Image() {}

    struct CreateParms_t
    {
        VkImageUsageFlags usageFlags;
        VkFormat format;
        int width;
        int height;
        int depth;
    };

    bool Create(DeviceContext *device, const CreateParms_t &parms);
    void Cleanup(DeviceContext *device);
    void TransitionLayout(DeviceContext *device);
    void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);
    void TransitionLayoutEN(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

    CreateParms_t m_parms;
    VkImage m_vkImage;
    VkImageView m_vkImageView;
    VkDeviceMemory m_vkDeviceMemory;

    VkImageLayout m_vkImageLayout;
};

namespace ElecNeko
{
    class CubeImage
    {
    public:
        CubeImage() = default;
        ~CubeImage() = default;

        bool Create(DeviceContext *device, const int width, const int height);
        void Cleanup(DeviceContext *device);
        void TransitionLayout(DeviceContext *device);
        void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

    public:
        VkImage m_vkImage;
        VkImageView m_vkImageView;
        VkDeviceMemory m_vkDeviceMemory;

        VkImageLayout m_vkImageLayout;
    };

    class ArrayImage
    {
    public:
        ArrayImage() = default;
        ~ArrayImage() = default;

        struct CreateParms_t
        {
            VkImageUsageFlags usageFlags;
            VkFormat format;
            int width;
            int height;
            int arrayLayers;
        };

        bool Create(DeviceContext *device, const CreateParms_t &parms);
        void Cleanup(DeviceContext *device);
        void TransitionLayout(DeviceContext *device);
        void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

    public:
        CreateParms_t m_parms;

        VkImage m_vkImage;
        VkImageView m_vkImageView;
        VkDeviceMemory m_vkDeviceMemory;

        VkImageLayout m_vkImageLayout;
    };
} // namespace ElecNeko
