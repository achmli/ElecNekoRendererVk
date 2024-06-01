#include "DeviceContext.hpp"
#include "Fence.hpp"
#include <cassert>

PFN_vkCreateDebugReportCallbackEXT vfs::vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vfs::vkDestroyDebugReportCallbackEXT;

void vfs::Link(VkInstance instance)
{
	vfs::vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackExt");
	vfs::vkDestroyDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackExt");
}

bool PhysicalDeviceProperties::AcquireProperties(VkPhysicalDevice device, VkSurfaceKHR surface)
{
	VkResult ret;

	vkPhysicalDevice = device;

	vkGetPhysicalDeviceProperties(vkPhysicalDevice, &vkDeviceProperties);
	vkGetPhysicalDeviceMemoryProperties(vkPhysicalDevice, &vkMemoryProperties);
	vkGetPhysicalDeviceFeatures(vkPhysicalDevice, &vkFeatures);

	// VkSurfaceCapabilitiesKHR
	ret = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(vkPhysicalDevice, surface, &vkSurfaceCapabilities);
	if (ret != VK_SUCCESS)
	{
		std::printf("Error: Failed to Get Physical Device Surface capabilities!\n");
		assert(0);
		return false;
	}

	// VkSurfaceFormatKHR
	{
		uint32_t numFormats;
		ret = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &numFormats, nullptr);
		if (ret != VK_SUCCESS || numFormats == 0)
		{
			std::printf("Error: Failed to get physical device surface formats num!\n");
			assert(0);
			return false;
		}

		vkSurfaceFormats.resize(numFormats || numFormats == 0);
		ret = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &numFormats, vkSurfaceFormats.data());
		if (ret != VK_SUCCESS)
		{
			std::printf("Error: Failed to get physical device surface formats!\n");
			assert(0);
			return false;
		}
	}

	// VkPresentModeKHR
	{
		uint32_t numModes;
		ret = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &numModes, nullptr);
		if (ret != VK_SUCCESS || numModes == 0)
		{
			std::printf("Error: Failed to get physical device surface present modes num!\n");
			assert(0);
			return false;
		}

		vkPresentModes.resize(numModes);
		ret = vkGetPhysicalDeviceSurfacePresentModesKHR(vkPhysicalDevice, surface, &numModes, vkPresentModes.data());
		if (ret != VK_SUCCESS || numModes == 0)
		{
			std::printf("Error: Failed to get physical device surface present modes!\n");
			assert(0);
			return false;
		}
	}

	// VkQueueFamilyProperties
	{
		uint32_t numQueues;
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &numQueues, nullptr);
		if (numQueues == 0)
		{
			std::printf("Error: Failed to get physical device queue family properties num!\n");
			assert(0);
			return false;
		}

		vkQueueFamilyProperties.resize(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(vkPhysicalDevice, &numQueues, vkQueueFamilyProperties.data());
		if (numQueues == 0)
		{
			std::printf("Error: Failed to get physical device queue family properties!\n");
			assert(0);
			return false;
		}
	}

	// VkExtensionProperties
	{
		uint32_t numExtensions;
		ret = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &numExtensions, nullptr);
		if (numExtensions == 0 || ret != VK_SUCCESS)
		{
			std::printf("Error:Failed to enumerate device extension properties num!\n");
			assert(0);
			return false;
		}

		vkExtensionProperties.resize(numExtensions);
		ret = vkEnumerateDeviceExtensionProperties(vkPhysicalDevice, nullptr, &numExtensions, vkExtensionProperties.data());
		if (numExtensions == 0 || ret != VK_SUCCESS)
		{
			std::printf("Error:Failed to enumerate device extension properties!\n");
			assert(0);
			return false;
		}
	}

	return true;
}

bool PhysicalDeviceProperties::HasExtensions(const char** extensions, const int num)const
{
	for (int i = 0; i < num; i++)
	{
		const char* extension = extensions[i];

		bool bExist = false;
		for (int j = 0; j < vkExtensionProperties.size(); j++)
		{
			if (strcmp(extension, vkExtensionProperties[j].extensionName) == 0)
			{
				bExist = true;
				break;
			}
		}

		if (!bExist)
		{
			return false;
		}
	}

	return true;
}

const std::vector<const char*>DeviceContext::deviceExtensions = {
	VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME
};

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanErrorMessage(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	std::printf("Error: VulkanErrorMessage: %s\n", msg);
	assert(0);
	return VK_FALSE;
}

bool DeviceContext::CreateInstance(bool enableLayers, const std::vector<const char*>& extensionsRequired)
{
	VkResult ret;

	bEnableLayers = enableLayers;

	std::vector<const char*> extensions = extensionsRequired;
	extensions.emplace_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	std::vector<const char*> validationLayers;
	if(bEnableLayers)
	{
		validationLayers = vkValidationLayers;

		uint32_t numLayers;
		vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

		std::vector<VkLayerProperties> layerProperties(numLayers);
		vkEnumerateInstanceLayerProperties(&numLayers, layerProperties.data());

		for(int i=0; i < numLayers; i++)
		{
			std::printf("Layer: %i %s\n", i, layerProperties[i].layerName);

			if(strcmp("VK_LAYER_KHRONOS_validation",layerProperties[i].layerName)==0)
			{
				validationLayers.clear();
				validationLayers.push_back("VK_LAYER_KHRONOS_validation");
				break;
			}
			if(strcmp("VK_LAYER_LUNARG_standard_validation",layerProperties[i].layerName)==0)
			{
				validationLayers.emplace_back("VK_LAYER_LUNARG_standard_validation");
			}
		}
	}
	vkValidationLayers=validationLayers;

	// Vulkan instance
	{
		VkApplicationInfo applicationInfo={};
		applicationInfo.sType=VK_STRUCTURE_TYPE_APPLICATION_INFO;
		applicationInfo.pApplicationName="Electric Neko Surface";
		applicationInfo.applicationVersion=VK_MAKE_VERSION(1, 0, 0);
		applicationInfo.apiVersion=VK_MAKE_VERSION(1, 0, 0);

		VkInstanceCreateInfo createInfo={};
		createInfo.sType=VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
		createInfo.pApplicationInfo=&applicationInfo;
		createInfo.enabledExtensionCount=(uint32_t)extensions.size();
		createInfo.ppEnabledExtensionNames=extensions.data();
		createInfo.enabledLayerCount=(uint32_t)validationLayers.size();
		createInfo.ppEnabledLayerNames=validationLayers.data();

		ret=vkCreateInstance(&createInfo,nullptr,&vkInstance);
		if(ret!=VK_SUCCESS)
		{
			std::printf("Error: Failed to create instance!\n");
			assert(0);
			return false;
		}
		vfs::Link(vkInstance);
	}

	if(enableLayers)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo={};
		createInfo.sType=VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags=VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback=VulkanErrorMessage;

		ret=vkCreateDebugReportCallbackEXT(vkInstance,&createInfo,nullptr,&vkDebugCallback);
		if(ret!=VK_SUCCESS)
		{
			std::printf("Error: Failed to create debug report callback!\n");
			assert(0);
			return false;
		}
	}

	return true;
}

void DeviceContext::Cleanup()
{
	
}

