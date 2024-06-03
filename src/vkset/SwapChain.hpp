#pragma once

#include <vulkan/vulkan.hpp>
#include <vector>

class DeviceContext;

class SwapChain
{
public:
	bool Create(DeviceContext* device, int width, int height);
	void Cleanup(DeviceContext* device);
	void Resize(DeviceContext* device, int width, int height);

	uint32_t BeginFrame(DeviceContext* device);
	void EndFrame(DeviceContext* device);

	void BeginRenderPass(DeviceContext* device);
	void EndRenderPass(DeviceContext* device);
public:
	VkSemaphore vkImageAvailableSemaphore;
	VkSemaphore vkRenderFinishedSemaphore;

	int32_t windowHeight;
	int32_t windowWidth;

	VkSwapchainKHR vkSwapChain;
	VkExtent2D vkExt;

	uint32_t currentImageIndex;
	VkFormat vkColorImageFormat;
	std::vector<VkImage> vkColorImages;
	std::vector<VkImageView> vkImageViews;

	VkFormat vkDepthFormat;
	VkImage vkDepthImage;
	VkImageView vkDepthImageView;
	VkDeviceMemory vkDepthImageMemory;

	std::vector<VkFramebuffer> vkFramebuffers;

	VkRenderPass vkRenderPass;
};
