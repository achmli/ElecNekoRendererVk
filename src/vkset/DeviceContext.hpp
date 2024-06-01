#pragma once

#include "SwapChain.hpp"
#include <vector>

/** Vulkan extension function */
class vfs
{
public:
	static void Link(VkInstance instance);

	static PFN_vkCreateDebugReportCallbackEXT vkCreateDebugReportCallbackEXT;
	static PFN_vkDestroyDebugReportCallbackEXT vkDestroyDebugReportCallbackEXT;
};

/** Physical Device Properties */
class PhysicalDeviceProperties
{
public:
	VkPhysicalDevice vkPhysicalDevice;
	VkPhysicalDeviceProperties vkDeviceProperties;
	VkPhysicalDeviceMemoryProperties vkMemoryProperties;
	VkPhysicalDeviceFeatures vkFeatures;
	VkSurfaceCapabilitiesKHR vkSurfaceCapabilities;
	std::vector<VkSurfaceFormatKHR> vkSurfaceFormats;
	std::vector<VkPresentModeKHR> vkPresentModes;
	std::vector<VkQueueFamilyProperties> vkQueueFamilyProperties;
	std::vector<VkExtensionProperties> vkExtensionProperties;

	bool AcquireProperties(VkPhysicalDevice device, VkSurfaceKHR surface);
	bool HasExtensions(const char** extensions, const int num)const;
};

/** DeviceContext */
class DeviceContext
{
public:
	bool CreateInstance(bool enableLayers, const std::vector<const char*>& extensions);
	void Cleanup();

	bool bEnableLayers;
	VkInstance vkInstance;
	VkDebugReportCallbackEXT vkDebugCallback;

	VkSurfaceKHR vkSurface;

	bool CreateDevice();
	bool CreatePhysicalDevice();
	bool CreateLogicalDevice();

	std::vector<PhysicalDeviceProperties> physicalDevices;

	// Device related
	int32_t deviceIndex;
	VkPhysicalDevice vkPhysicalDevice;
	VkDevice vkDevice;

	int32_t graphicsFamilyIdx;
	int32_t presentFamilyIdx;

	uint32_t FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties);

	static const std::vector<const char*> deviceExtensions;
	std::vector<const char*> vkValidationLayers;

	// Command buffers
	bool CreateCommandBuffers();

	VkCommandPool vkCommandPool;
	std::vector<VkCommandBuffer> vkCommandBuffers;

	VkCommandBuffer CreateCommandBuffer(VkCommandBufferLevel level);
	void FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue);

	// Swapchain related
	SwapChain swapChain;
	bool CreateSwapChain(int32_t width, int32_t height) { return swapChain.Create(this, width, height); }
	void ResizeWindow(int32_t width, int32_t height) { swapChain.Resize(this, width, height); }

	uint32_t BeginFrame() { return swapChain.BeginFrame(this); }
	void EndFrame() { swapChain.EndFrame(this); }

	void BeginRenderPass() { swapChain.BeginRenderPass(this); }
	void EndRenderPass() { swapChain.EndRenderPass(this); }

	int32_t GetAlignedUniformByteOffset(const int32_t offset)const;
};