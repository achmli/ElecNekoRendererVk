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
	VkSemaphore ImageAvailableSemaphore;
	VkSemaphore RenderFinishedSemaphore;

	int32_t windowHeight;
	int32_t windowWidth;

	VkSwapchainKHR swapChain;
	VkExtent2D ext;

	uint32_t currentImageIndex;
	VkFormat colorImageFormat;
	std::vector<VkImage> colorImages;
	std::vector<VkImageView> imageViews;

	VkFormat depthFormat;
	VkImage depthImage;
	VkImageView depthImageView;
	VkDeviceMemory depthImageMemory;

	std::vector<VkFramebuffer> framebuffers;

	VkRenderPass renderPass;
};
