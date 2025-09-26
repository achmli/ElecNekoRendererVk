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

        bool Create(DeviceContext *device, const int width, const int height, const int mipLevels = 1, VkFormat format = VK_FORMAT_R8G8B8A8_UNORM);
        void Cleanup(DeviceContext *device);
        void TransitionLayout(DeviceContext *device, VkImageLayout newLayout = VK_IMAGE_LAYOUT_GENERAL);
        void TransitionLayout(VkCommandBuffer cmdBuffer, VkImageLayout newLayout);

        void TransitionMipLayout(VkCommandBuffer cmdBuffer, int mip, VkImageLayout newLayout);

        VkImageView GetFaceView(int face, int mip) const
        {
            const int index = mip * 6 + face;
            if (index < 0 || index >= (int) m_faceMipViews.size())
                return VK_NULL_HANDLE;
            return m_faceMipViews[index];
        }

    public:
        VkImage m_vkImage = VK_NULL_HANDLE;

        VkImageView m_vkImageView = VK_NULL_HANDLE;
        std::vector<VkImageView> m_faceMipViews;

        VkDeviceMemory m_vkDeviceMemory = VK_NULL_HANDLE;

        VkImageLayout m_vkImageLayout = VK_IMAGE_LAYOUT_UNDEFINED;

        std::vector<VkImageLayout> m_mipLayouts;

        int m_width = 0;
        int m_height = 0;
        int m_mipLevels = 1;
        VkFormat m_format = VK_FORMAT_UNDEFINED;
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
