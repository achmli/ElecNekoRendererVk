//
//  FrameBuffer.h
//
#pragma once
#include <vulkan/vulkan.hpp>
#include "Image.h"

class DeviceContext;

/*
====================================================
FrameBuffer
====================================================
*/
class FrameBuffer {
public:
	FrameBuffer() {}
	~FrameBuffer() {}

	struct CreateParms_t {
		int width;
		int height;
		bool hasDepth;
		bool hasColor;
	};

	bool Create( DeviceContext * device, CreateParms_t & parms );
	void Cleanup( DeviceContext * device );

	void BeginRenderPass( DeviceContext * device, const int cmdBufferIndex );
	void EndRenderPass( DeviceContext * device, const int cmdBufferIndex );

	CreateParms_t			m_parms;

	Image					m_imageDepth;
	Image					m_imageColor;

	VkFramebuffer			m_vkFrameBuffer;
	VkRenderPass			m_vkRenderPass;

private:
	bool CreateRenderPass( DeviceContext * device );
};

namespace ElecNeko
{
	class GFrameBuffer
	{
    public:
        GFrameBuffer() = default;
        ~GFrameBuffer() = default;

		struct CreateParms_t
		{
            int width;
            int height;
		};

		bool Create(DeviceContext *device, CreateParms_t &parms);
        void Cleanup(DeviceContext *device);

        void BeginRenderPass(DeviceContext *device, const int cmdBufferIndex);
        void EndRenderPass(DeviceContext *device, const int cmdBufferIndex);

		CreateParms_t m_parms;

		Image m_imageNormal;
        Image m_imageAlbedo;
        Image m_imageDepth;

		VkFramebuffer m_vkFrameBuffer;
        VkRenderPass m_vkRenderPass;

    private:
        bool CreateRenderPass(DeviceContext *device);
	};
}
