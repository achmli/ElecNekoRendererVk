#include "DeviceContext.h"
#include "Fence.h"
#include<assert.h>

/** vfs */

PFN_vkCreateDebugReportCallbackEXT vfs::vkCreateDebugReportCallbackEXT;
PFN_vkDestroyDebugReportCallbackEXT vfs::vkDestoryDebugReportCallbackEXT;

void vfs::Link(VkInstance instance)
{
	vfs::vkCreateDebugReportCallbackEXT = (PFN_vkCreateDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugReportCallbackEXT");
	vfs::vkDestoryDebugReportCallbackEXT = (PFN_vkDestroyDebugReportCallbackEXT)vkGetInstanceProcAddr(instance, "vkDestroyDebugReportCallbackEXT");
}

/** Physical Device Properties */

bool PhysicalDeviceProperties::AcquireProperties(VkPhysicalDevice device, VkSurfaceKHR vkSurface)
{
	VkResult result;

	m_vkPhysicalDevice = device;

	vkGetPhysicalDeviceProperties(m_vkPhysicalDevice, &m_vkDeviceProperties);
	vkGetPhysicalDeviceMemoryProperties(m_vkPhysicalDevice, &m_vkMemoryProperties);
	vkGetPhysicalDeviceFeatures(m_vkPhysicalDevice, &m_vkFeatures);

	// VkSurfaceCapabilitiesKHR
	result = vkGetPhysicalDeviceSurfaceCapabilitiesKHR(m_vkPhysicalDevice, vkSurface, &m_vkSurfaceCapabilities);
	if (result != VK_SUCCESS)
	{
		printf("ERROR: Failed to Get Physical Device Surface Capabilities!\n");
		assert(0);
		return false;
	}

	// VkSurfaceFormatKHR
	{
		uint32_t numFormats;
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, vkSurface, &numFormats, NULL);
		if (result != VK_SUCCESS || numFormats == 0)
		{
			printf("ERROR: Failed to Get Physical Device Surface Formats!\n");
			assert(0);
			return false;
		}

		m_vkSurfaceFormats.resize(numFormats);
		result = vkGetPhysicalDeviceSurfaceFormatsKHR(m_vkPhysicalDevice, vkSurface, &numFormats, m_vkSurfaceFormats.data());
		if (result != VK_SUCCESS || numFormats == 0)
		{
			printf("ERROR: Failed to Get Physical Device Surface Formats!\n");
			assert(0);
			return false;
		}
	}

	// VkPresentModeKHR
	{
		uint32_t numPresentModes;
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, vkSurface, &numPresentModes, NULL);
		if (result != VK_SUCCESS || numPresentModes == 0)
		{
			printf("ERROR: Failed to Get Physical Device Surface Present Modes!\n");
			assert(0);
			return false;
		}

		m_vkPresentModes.resize(numPresentModes);
		result = vkGetPhysicalDeviceSurfacePresentModesKHR(m_vkPhysicalDevice, vkSurface, &numPresentModes, m_vkPresentModes.data());
		if (result != VK_SUCCESS || numPresentModes == 0)
		{
			printf("ERROR: Failed to Get Physical Device Surface Present Modes!\n");
			assert(0);
			return false;
		}
	}

	//VkQueueFamilyProperties
	{
		uint32_t numQueues = 0;
		vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &numQueues, NULL);
		if (numQueues == 0)
		{
			printf("ERROR: Failed to Get Physical Device Queue Family Properties!\n");
			assert(0);
			return false;
		}

		m_vkQueueFamilyProperties.resize(numQueues);
		vkGetPhysicalDeviceQueueFamilyProperties(m_vkPhysicalDevice, &numQueues, m_vkQueueFamilyProperties.data());
		if (numQueues == 0)
		{
			printf("ERROR: Failed to Get Physical Device Queue Family Properties!\n");
			assert(0);
			return false;
		}
	}

	// VkExtensionProperties
	{
		uint32_t numExtensions;
		result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, NULL, &numExtensions, NULL);
		if (result != VK_SUCCESS || numExtensions == 0)
		{
			printf("ERROR: Failed to Enumerate Device Extension Properties!\n");
			assert(0);
			return false;
		}

		m_vkExtensionProperties.resize(numExtensions);
		result = vkEnumerateDeviceExtensionProperties(m_vkPhysicalDevice, NULL, &numExtensions, m_vkExtensionProperties.data());
		if (result != VK_SUCCESS || numExtensions == 0)
		{
			printf("ERROR: Failed to Enumerate Device Extension Properties!\n");
			assert(0);
			return false;
		}
	}

	return true;
}

bool PhysicalDeviceProperties::HasExtensions(const char** extensions, const int num) const
{
	for (int i = 0; i < num; i++)
	{
		const char* extension = extensions[i];

		bool doseExist = false;
		for (int j = 0; j < m_vkExtensionProperties.size(); j++)
		{
			if (strcmp(extension, m_vkExtensionProperties[j].extensionName) == 0)
			{
				doseExist = true;
				break;
			}
		}

		if (!doseExist)
		{
			return false;
		}
	}

	return true;
}

/** Device Context */

const std::vector<const char*> DeviceContext::m_deviceExtensions = { VK_KHR_SWAPCHAIN_EXTENSION_NAME,VK_KHR_GET_MEMORY_REQUIREMENTS_2_EXTENSION_NAME };

/** Vulkan Error Message */

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanErrorMessage(VkDebugReportFlagsEXT flags, VkDebugReportObjectTypeEXT objType, uint64_t obj, size_t location, int32_t code, const char* layerPrefix, const char* msg, void* userData)
{
	printf("ERROR: VulkanErrorMessage:%s\n", msg);
	assert(0);

	return VK_FALSE;
}

bool DeviceContext::CreateInstance(bool enableLayers, const std::vector<const char*>& extensions_required)
{
	VkResult result;

	m_enableLayers = enableLayers;

	std::vector<const char*> extensions = extensions_required;
	extensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

	std::vector<const char*> validationLayers;
	if (m_enableLayers)
	{
		validationLayers = m_validationLayers;

		uint32_t numLayers;
		vkEnumerateInstanceLayerProperties(&numLayers, nullptr);

		std::vector<VkLayerProperties> layerProperties(numLayers);
		vkEnumerateInstanceLayerProperties(&numLayers, layerProperties.data());

		for (int i = 0; i < numLayers; i++)
		{
			printf("Layer: %i %s\n", i, layerProperties[i].layerName);

			if (strcmp("VK_LAYER_KHRONOS_validation", layerProperties[i].layerName) == 0)
			{
				validationLayers.clear();
				validationLayers.push_back("VK_LAYER_KHRONOS_validation");
				break;
			}
			if (strcmp("VK_LAYER_LUNARG_standard_validation", layerProperties[i].layerName) == 0)
			{
				validationLayers.push_back("VK_LAYER_LUNARG_standard_validation");
			}
		}
	}

	m_validationLayers = validationLayers;

	// Vulkan Instance

	VkApplicationInfo appInfo = {};
	appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
	appInfo.pApplicationName = "ElecNekoSurface";
	appInfo.applicationVersion = VK_MAKE_VERSION(0, 1, 0);
	appInfo.apiVersion = VK_MAKE_VERSION(0, 1, 0);

	VkInstanceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
	createInfo.pApplicationInfo = &appInfo;
	createInfo.enabledExtensionCount = (uint32_t)extensions.size();
	createInfo.ppEnabledExtensionNames = extensions.data();
	createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	result = vkCreateInstance(&createInfo, nullptr, &m_vkInstance);
	if (result != VK_SUCCESS)
	{
		printf("ËRROR: Failed to craete vulkan instance!\n");
		assert(0);
		return false;
	}
	vfs::Link(m_vkInstance);

	if (m_enableLayers)
	{
		VkDebugReportCallbackCreateInfoEXT createInfo = {};
		createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT;
		createInfo.flags = VK_DEBUG_REPORT_ERROR_BIT_EXT | VK_DEBUG_REPORT_WARNING_BIT_EXT;
		createInfo.pfnCallback = VulkanErrorMessage;

		result = vfs::vkCreateDebugReportCallbackEXT(m_vkInstance, &createInfo, nullptr, &m_vkDebugCallback);
		if (result != VK_SUCCESS)
		{
			printf("ERROR: Failed to create debug report callback!\n");
			assert(0);
			return false;
		}
	}

	return true;
}

void DeviceContext::Cleanup()
{
	m_swapChain.Cleanup(this);

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, (uint32_t)m_vkCommandBuffers.size(), m_vkCommandBuffers.data());
	vkDestroyCommandPool(m_vkDevice, m_vkCommandPool, nullptr);

	vkDestroyDevice(m_vkDevice, nullptr);

	if (m_enableLayers)
	{
		vfs::vkDestoryDebugReportCallbackEXT(m_vkInstance, m_vkDebugCallback, nullptr);
	}
	vkDestroySurfaceKHR(m_vkInstance, m_vkSurface, nullptr);
	vkDestroyInstance(m_vkInstance, nullptr);
}

bool DeviceContext::CreateDevice()
{
	if (!CreatePhysicalDevice())
	{
		printf("ERROR: Failed to create physical device!\n");
		assert(0);
		return false;
	}

	if (!CreateLogicalDevice())
	{
		printf("ERROR: Failed to create logical device!\n");
		assert(0);
		return false;
	}

	return true;
}

const char* VendorStr(unsigned int VendorID)
{
	switch (VendorID)
	{
	case 0x1002:
		return "AMD";
	case 0x1010:
		return "ImgTec";
	case 0x10DE:
		return "NVIDIA";
	case 0x13B5:
		return "ARM";
	case 0x5143:
		return "Qualcomn";
	case 0x8086:
		return "INTEL";
	default:
		return "";
	}
}

bool DeviceContext::CreatePhysicalDevice()
{
	VkResult result;

	//Enumerate Physical Device
	{
		// get number of devcices
		uint32_t numDevices = 0;
		result = vkEnumeratePhysicalDevices(m_vkInstance, &numDevices, NULL);
		if (result != VK_SUCCESS || numDevices == 0)
		{
			printf("ERROR: Failed to enumerate physical devices!\n");
			assert(0);
			return false;
		}

		// Query a list of devices
		std::vector<VkPhysicalDevice> physicalDevices(numDevices);
		result = vkEnumeratePhysicalDevices(m_vkInstance, &numDevices, physicalDevices.data());
		if (result != VK_SUCCESS || numDevices == 0)
		{
			printf("ERROR: Failed to enumerate physical devices!\n");
			assert(0);
			return false;
		}

		// acquire the properties of each device
		m_physicalDevices.resize(physicalDevices.size());
		for (uint32_t i = 0; i < physicalDevices.size(); i++)
		{
			m_physicalDevices[i].AcquireProperties(physicalDevices[i], m_vkSurface);
		}
	}

	// Select a physical device
	// Todo: it should could be selected by user
	for (int i = 0; i < m_physicalDevices.size(); i++)
	{
		const PhysicalDeviceProperties& deviceProperties = m_physicalDevices[i];

		// ignore non_drawing device
		if (deviceProperties.m_vkPresentModes.size() == 0)
		{
			continue;
		}
		if (deviceProperties.m_vkSurfaceFormats.size() == 0)
		{
			continue;
		}

		// Verify required extension support
		if (!deviceProperties.HasExtensions((const char**)m_deviceExtensions.data(), (int)m_deviceExtensions.size()))
		{
			continue;
		}

		// find graphics queue
		int graphicsIdx = -1;
		for (int j = 0; j < deviceProperties.m_vkQueueFamilyProperties.size(); ++j)
		{
			const VkQueueFamilyProperties& props = deviceProperties.m_vkQueueFamilyProperties[j];

			if (props.queueCount == 0)
			{
				continue;
			}

			if (props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
			{
				graphicsIdx = j;
				break;
			}
		}
		// device do not support graphics
		if (graphicsIdx < 0)
		{
			continue;
		}

		// find present queue family
		int presentIdx = -1;
		for (int j = 0; j < deviceProperties.m_vkQueueFamilyProperties.size(); ++j)
		{
			const VkQueueFamilyProperties& props = deviceProperties.m_vkQueueFamilyProperties[j];

			if (props.queueCount == 0)
			{
				continue;
			}

			VkBool32 supportsPresentQueue = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR(deviceProperties.m_vkPhysicalDevice, j, m_vkSurface, &supportsPresentQueue);
			if (supportsPresentQueue)
			{
				presentIdx = j;
				break;
			}
		}
		// device do not support presentation
		if (presentIdx < 0)
		{
			continue;
		}

		// Use first device supports graphics and presentation
		m_graphicsFamilyIdx = graphicsIdx;
		m_presentFamilyIdx = presentIdx;
		m_vkPhysicalDevice = deviceProperties.m_vkPhysicalDevice;
		m_deviceIndex = i;

		uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.m_vkDeviceProperties.apiVersion);
		uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.m_vkDeviceProperties.apiVersion);
		uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.m_vkDeviceProperties.apiVersion);

		uint32_t driverMajor = VK_VERSION_MAJOR(deviceProperties.m_vkDeviceProperties.driverVersion);
		uint32_t driverMinor = VK_VERSION_MINOR(deviceProperties.m_vkDeviceProperties.driverVersion);
		uint32_t driverPatch = VK_VERSION_PATCH(deviceProperties.m_vkDeviceProperties.driverVersion);

		printf("Physical Device Chosen: %s\n", deviceProperties.m_vkDeviceProperties.deviceName);
		printf("API Version: %i.%i.%i\n", apiMajor, apiMinor, apiPatch);
		printf("Driver Version: %i.%i.%i\n", driverMajor, driverMinor, driverPatch);
		printf("Vendor ID: %i %s\n", deviceProperties.m_vkDeviceProperties.vendorID, VendorStr(deviceProperties.m_vkDeviceProperties.vendorID));
		printf("Device ID: %i\n", deviceProperties.m_vkDeviceProperties.deviceID);
		return true;
	}

	printf("ERROR: Failed to create physical device!\n");
	assert(0);
	return false;
}

bool DeviceContext::CreateLogicalDevice()
{
	VkResult result;

	std::vector<const char*> validationLayers;
	if (m_enableLayers)
	{
		validationLayers = m_validationLayers;
	}

	float queuePriority = 1.f;
	VkDeviceQueueCreateInfo queueCreateInfo[2] = {};
	queueCreateInfo[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[0].queueFamilyIndex = m_graphicsFamilyIdx;
	queueCreateInfo[0].queueCount = 1;
	queueCreateInfo[0].pQueuePriorities = &queuePriority;

	queueCreateInfo[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfo[1].queueFamilyIndex = m_presentFamilyIdx;
	queueCreateInfo[1].queueCount = 1;
	queueCreateInfo[1].pQueuePriorities = &queuePriority;

	VkPhysicalDeviceFeatures deviceFeatures = {};
	deviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo createInfo = {};
	createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
	if (m_presentFamilyIdx != m_graphicsFamilyIdx)
	{
		createInfo.queueCreateInfoCount = 2;
	}
	else
	{
		createInfo.queueCreateInfoCount = 1;
	}
	createInfo.pQueueCreateInfos = queueCreateInfo;
	createInfo.pEnabledFeatures = &deviceFeatures;
	createInfo.enabledExtensionCount = (uint32_t)m_deviceExtensions.size();
	createInfo.ppEnabledExtensionNames = m_deviceExtensions.data();
	createInfo.enabledLayerCount = (uint32_t)validationLayers.size();
	createInfo.ppEnabledLayerNames = validationLayers.data();

	result = vkCreateDevice(m_vkPhysicalDevice, &createInfo, nullptr, &m_vkDevice);
	if (result != VK_SUCCESS)
	{
		printf("Failed to create logical device!\n");
		assert(0);
		return false;
	}

	vkGetDeviceQueue(m_vkDevice, m_graphicsFamilyIdx, 0, &m_vkGraphicsQueue);
	vkGetDeviceQueue(m_vkDevice, m_presentFamilyIdx, 0, &m_vkPresentQueue);

	return true;
}

uint32_t DeviceContext::FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
	VkPhysicalDeviceMemoryProperties memProperties = m_physicalDevices[m_deviceIndex].m_vkMemoryProperties;

	for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++)
	{
		if (typeFilter & (1 << i) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties)
		{
			return i;
		}
	}

	printf("ERROR: Failed to find memory type index!\n");
	assert(0);
	return 0;
}

bool DeviceContext::CreateCommandBuffers()
{
	VkResult result;

	// Command pool
	{
		VkCommandPoolCreateInfo poolInfo = {};
		poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		poolInfo.queueFamilyIndex = m_graphicsFamilyIdx;
		poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

		result = vkCreateCommandPool(m_vkDevice, &poolInfo, nullptr, &m_vkCommandPool);
		if (result != VK_SUCCESS)
		{
			printf("Failed to create command pool!\n");
			assert(0);
			return false;
		}
	}

	// Command buffers
	{
		const int numBuffers = 16;
		m_vkCommandBuffers.resize(numBuffers);

		VkCommandBufferAllocateInfo allocInfo = {};
		allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		allocInfo.commandPool = m_vkCommandPool;
		allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		allocInfo.commandBufferCount = (uint32_t)m_vkCommandBuffers.size();

		result = vkAllocateCommandBuffers(m_vkDevice, &allocInfo, m_vkCommandBuffers.data());
		if (result != VK_SUCCESS)
		{
			printf("Failed to allocate command buffers!\n");
			assert(0);
			return false;
		}
	}

	return true;
}

VkCommandBuffer DeviceContext::CreateCommandBuffer(VkCommandBufferLevel level)
{
	VkResult result;

	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.commandPool = m_vkCommandPool;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer cmdBuffer;
	result = vkAllocateCommandBuffers(m_vkDevice, &allocInfo, &cmdBuffer);
	if (result != VK_SUCCESS)
	{
		printf("Failed to allocate command buffer!\n");
		assert(0);
	}

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	result = vkBeginCommandBuffer(cmdBuffer, &beginInfo);
	if (result != VK_SUCCESS)
	{
		printf("Failed to begin command buffer!\n");
		assert(0);
	}

	return cmdBuffer;
}

void DeviceContext::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
{
	if (commandBuffer == VK_NULL_HANDLE)
	{
		return;
	}

	{
		Fence fence(this);

		VkSubmitInfo submitInfo{};
		submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		submitInfo.commandBufferCount = 1;
		submitInfo.pCommandBuffers = &commandBuffer;

		vkQueueSubmit(queue, 1, &submitInfo, fence.m_vkFence);
	}

	vkFreeCommandBuffers(m_vkDevice, m_vkCommandPool, 1, &commandBuffer);
}

/**
* Devices have a minimum byte alignment for their offsets.
* This function converts the incoming byte offset to a proper byte alignment
*/
int DeviceContext::GetAlignedUniformByteOffset(const int offset) const
{
	const PhysicalDeviceProperties& deviceProperties = m_physicalDevices[m_deviceIndex];
	const int minByteOffsetAlignment = deviceProperties.m_vkDeviceProperties.limits.minUniformBufferOffsetAlignment;

	const int n = (offset + minByteOffsetAlignment - 1) / minByteOffsetAlignment;
	const int alignedOffset = n * minByteOffsetAlignment;
	return alignedOffset;
}