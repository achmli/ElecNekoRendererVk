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

		vkSurfaceFormats.resize(numFormats);
		ret = vkGetPhysicalDeviceSurfaceFormatsKHR(vkPhysicalDevice, surface, &numFormats, vkSurfaceFormats.data());
		if (ret != VK_SUCCESS || numFormats == 0)
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
		for (const auto & vkExtensionProperty : vkExtensionProperties)
		{
			if (strcmp(extension, vkExtensionProperty.extensionName) == 0)
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

		for(uint32_t i=0; i < numLayers; i++)
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

		ret = vkCreateInstance(&createInfo, nullptr, &vkInstance);
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

		ret = vfs::vkCreateDebugReportCallbackEXT(vkInstance, &createInfo, nullptr, &vkDebugCallback);
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
	swapChain.Cleanup(this);

    // Destroy Command Buffers
    vkFreeCommandBuffers(vkDevice,vkCommandPool,(uint32_t)vkCommandBuffers.size(),vkCommandBuffers.data());
    vkDestroyCommandPool(vkDevice,vkCommandPool,nullptr);

    if(bEnableLayers)
    {
        vfs::vkDestroyDebugReportCallbackEXT(vkInstance,vkDebugCallback,nullptr);
    }
    vkDestroySurfaceKHR(vkInstance,vkSurface,nullptr);
    vkDestroyInstance(vkInstance,nullptr);
}

bool DeviceContext::CreateDevice()
{
    if(!CreatePhysicalDevice())
    {
        std::printf("Error: Failed to create physical device!\n");
        assert(0);
        return false;
    }

    if(!CreateLogicalDevice())
    {
        std::printf("Error: Failed to create logical device!\n");
        assert(0);
        return false;
    }

    return true;
}

const char* VendorStr(unsigned int vendorID) {
    if (vendorID == 0x1002)
    {
        return "AMD";
    }
    if (vendorID == 0x1010)
    {
        return "ImgTec";
    }
    if (vendorID == 0x10DE)
    {
        return "NVIDIA";
    }
    if (vendorID == 0x13B5)
    {
        return "ARM";
    }
    if (vendorID == 0x5143)
    {
      return "Qualcomm";
    }
    if (vendorID == 0x8086)
    {
      return "INTEL";
    }

    return "";
}

bool DeviceContext::CreatePhysicalDevice()
{
    VkResult ret;

    // Enumerate physical device
    {
        // Get the number of devices
        uint32_t numDevices=0;
        ret= vkEnumeratePhysicalDevices(vkInstance,&numDevices,nullptr);
        if(ret != VK_SUCCESS || numDevices == 0)
        {
            std::printf("Error: Failed to enumerate physical devices!\n");
            assert(0);
            return false;
        }

        // Query a list of devices
        std::vector<VkPhysicalDevice> physicalDevices(numDevices);
        ret = vkEnumeratePhysicalDevices(vkInstance,&numDevices,physicalDevices.data());
        if(ret != VK_SUCCESS || numDevices == 0)
        {
            std::printf("Error: Failed to enumerate physical devices!\n");
            assert(0);
            return false;
        }

        // Acquire the properties of each device
        vkPhysicalDevices.resize(physicalDevices.size());
        for(uint32_t i=0;i<physicalDevices.size();i++)
        {
            vkPhysicalDevices[i].AcquireProperties(physicalDevices[i],vkSurface);
        }
    }

    // Select a physical device
    // todo: imgui support
    for(int i=0;i<vkPhysicalDevices.size();i++)
    {
        const PhysicalDeviceProperties& deviceProperties = vkPhysicalDevices[i];

        // Ignore non-drawing device
        if(deviceProperties.vkPresentModes.empty())
        {
            continue;
        }
        if(deviceProperties.vkSurfaceFormats.empty())
        {
            continue;
        }

        // Verify required extension support
        if(!deviceProperties.HasExtensions((const char **)deviceExtensions.data(),(int)deviceExtensions.size()))
        {
            continue;
        }

        // Find graphics queue family
        int graphicsIdx=-1;
        for(int j=0;j<deviceProperties.vkQueueFamilyProperties.size();j++)
        {
            const VkQueueFamilyProperties& props=deviceProperties.vkQueueFamilyProperties[j];

            if(props.queueCount == 0)
            {
                continue;
            }

            if(props.queueFlags & VK_QUEUE_GRAPHICS_BIT)
            {
                graphicsIdx=j;
                break;
            }
        }
        if(graphicsIdx<0)
        {
            // Device does not support graphics
            continue;
        }

        // Find present queue family
        int presentIdx=-1;
        for(int j=0;j<deviceProperties.vkQueueFamilyProperties.size();j++)
        {
            const VkQueueFamilyProperties& props=deviceProperties.vkQueueFamilyProperties[j];

            if(props.queueCount==0)
            {
                continue;
            }

            VkBool32 supportsPresentQueue=VK_FALSE;
            vkGetPhysicalDeviceSurfaceSupportKHR(deviceProperties.vkPhysicalDevice,j,vkSurface,&supportsPresentQueue);
            if(supportsPresentQueue)
            {
                presentIdx=j;
                break;
            }
        }

        if(presentIdx<0)
        {
            // device does not support present
            continue;
        }

        // Use first device that that supports graphics and presentation
        // todo
        graphicsFamilyIdx = graphicsIdx;
        presentFamilyIdx = presentIdx;
        vkPhysicalDevice = deviceProperties.vkPhysicalDevice;
        deviceIndex = i;

        uint32_t apiMajor = VK_VERSION_MAJOR(deviceProperties.vkDeviceProperties.apiVersion);
        uint32_t apiMinor = VK_VERSION_MINOR(deviceProperties.vkDeviceProperties.apiVersion);
        uint32_t apiPatch = VK_VERSION_PATCH(deviceProperties.vkDeviceProperties.apiVersion);

        uint32_t driverMajor = VK_VERSION_MAJOR(deviceProperties.vkDeviceProperties.driverVersion);
        uint32_t driverMinor = VK_VERSION_MINOR(deviceProperties.vkDeviceProperties.driverVersion);
        uint32_t driverPatch = VK_VERSION_PATCH(deviceProperties.vkDeviceProperties.driverVersion);

        std::printf("Physical Device Chosen: %s\n",deviceProperties.vkDeviceProperties.deviceName);
        std::printf("API Version: %i.%i/%i\n",apiMajor,apiMinor,apiPatch);
        std::printf("Driver Version: %i.%i.%i\n",driverMajor,driverMinor,driverPatch);
        std::printf("Vendor ID: %i  %s\n",deviceProperties.vkDeviceProperties.vendorID, VendorStr(deviceProperties.vkDeviceProperties.vendorID));
        std::printf("Device ID: %i\n",deviceProperties.vkDeviceProperties.deviceID);
        return true;
    }

    std::printf("Error: Failed to create Physical device!\n");
    assert(0);
    return false;
}

bool DeviceContext::CreateLogicalDevice()
{
    VkResult ret;

    std::vector<const char*> validationLayers;
    if(bEnableLayers)
    {
        validationLayers=vkValidationLayers;
    }

    float queuePriority=1.0f;
    VkDeviceQueueCreateInfo queueCreateInfo[2]={};
    queueCreateInfo[0].sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[0].queueFamilyIndex=graphicsFamilyIdx;
    queueCreateInfo[0].queueCount=1;
    queueCreateInfo[0].pQueuePriorities=&queuePriority;

    queueCreateInfo[1].sType=VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    queueCreateInfo[1].queueFamilyIndex=presentFamilyIdx;
    queueCreateInfo[1].queueCount=1;
    queueCreateInfo[1].pQueuePriorities=&queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures={};
    deviceFeatures.samplerAnisotropy=VK_TRUE;

    VkDeviceCreateInfo createInfo={};
    createInfo.sType=VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    if(presentFamilyIdx!=graphicsFamilyIdx)
    {
        createInfo.queueCreateInfoCount=2;
    }
    else
    {
        createInfo.queueCreateInfoCount=1;
    }
    createInfo.pQueueCreateInfos=queueCreateInfo;
    createInfo.pEnabledFeatures=&deviceFeatures;
    createInfo.enabledExtensionCount=(uint32_t)deviceExtensions.size();
    createInfo.ppEnabledExtensionNames=deviceExtensions.data();
    createInfo.enabledLayerCount=(uint32_t)validationLayers.size();
    createInfo.ppEnabledLayerNames=validationLayers.data();

    ret=vkCreateDevice(vkPhysicalDevice,&createInfo, nullptr,&vkDevice);
    if(ret!=VK_SUCCESS)
    {
        std::printf("Error: Failed to create logical device!\n");
        assert(0);
        return false;
    }

    vkGetDeviceQueue(vkDevice,graphicsFamilyIdx,0,&vkGraphicsQueue);
    vkGetDeviceQueue(vkDevice,presentFamilyIdx,0,&vkPresentQueue);

    return true;
}

uint32_t DeviceContext::FindMemoryTypeIndex(uint32_t typeFilter, VkMemoryPropertyFlags properties)
{
    VkPhysicalDeviceMemoryProperties memoryProperties=vkPhysicalDevices[deviceIndex].vkMemoryProperties;

    for(uint32_t i=0;i<memoryProperties.memoryTypeCount;i++)
    {
        if((typeFilter & (1 << i)) && (memoryProperties.memoryTypes[i].propertyFlags & properties)==properties)
        {
            return i;
        }
    }

    std::printf("Error: Failed to find memory type index!\n");
    assert(0);
    return 0;
}

bool DeviceContext::CreateCommandBuffers()
{
    VkResult ret;

    // Command pool
    {
        VkCommandPoolCreateInfo poolInfo={};
        poolInfo.sType=VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.queueFamilyIndex=graphicsFamilyIdx;
        poolInfo.flags=VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;

        ret= vkCreateCommandPool(vkDevice,&poolInfo, nullptr,&vkCommandPool);
        if(ret!=VK_SUCCESS)
        {
            std::printf("Error: Failed to create command pool!\n");
            assert(0);
            return false;
        }
    }

    // Command Buffers
    {
        const int numBuffers=16;
        vkCommandBuffers.resize(numBuffers);

        VkCommandBufferAllocateInfo allocInfo={};
        allocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool=vkCommandPool;
        allocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount=(uint32_t)vkCommandBuffers.size();

        ret= vkAllocateCommandBuffers(vkDevice,&allocInfo,vkCommandBuffers.data());
        if(ret!=VK_SUCCESS)
        {
            std::printf("Error: Failed to allocate command buffers!\n");
            assert(0);
            return false;
        }
    }

    return true;
}

VkCommandBuffer DeviceContext::CreateCommandBuffer(VkCommandBufferLevel level) const
{
    VkResult ret;

    VkCommandBufferAllocateInfo allocInfo={};
    allocInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool=vkCommandPool;
    allocInfo.level=VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    allocInfo.commandBufferCount=1;

    VkCommandBuffer cmdBuffer;
    ret= vkAllocateCommandBuffers(vkDevice,&allocInfo,&cmdBuffer);
    if(ret!=VK_SUCCESS)
    {
        std::printf("Error: Failed to create command buffers!\n");
        assert(0);
    }

    // start the command buffer
    VkCommandBufferBeginInfo beginInfo={};
    beginInfo.sType=VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    ret= vkBeginCommandBuffer(cmdBuffer,&beginInfo);
    if(ret!=VK_SUCCESS)
    {
        std::printf("Error: Failed to begin command buffers!\n");
        assert(0);
    }

    return cmdBuffer;
}

void DeviceContext::FlushCommandBuffer(VkCommandBuffer commandBuffer, VkQueue queue)
{
    if(commandBuffer==VK_NULL_HANDLE)
    {
        return;
    }

    vkEndCommandBuffer(commandBuffer);

    {
        Fence fence(this);

        // submit to the queue
        VkSubmitInfo submitInfo {};
        submitInfo.sType=VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount=1;
        submitInfo.pCommandBuffers=&commandBuffer;

        vkQueueSubmit(queue,1,&submitInfo,fence.vkFence);
    }

    vkFreeCommandBuffers(vkDevice,vkCommandPool,1,&commandBuffer);
}

/**
 * Devices have a minimum byte alignment for their offsets.
 * This function converts the incoming byte offset to a proper byte alignment.
 * */
int DeviceContext::GetAlignedUniformByteOffset(const int32_t offset) const
{
     const PhysicalDeviceProperties& deviceProperties=vkPhysicalDevices[deviceIndex];
     const int minByteOffsetAlignment=static_cast<int>(deviceProperties.vkDeviceProperties.limits.minUniformBufferOffsetAlignment);

     const int n=(offset+minByteOffsetAlignment-1)/minByteOffsetAlignment;
     const int alignedOffset=n*minByteOffsetAlignment;
     return alignedOffset;
}