#include "Fence.hpp"
#include <cassert>
#include <cstdio>

bool Fence::Create(DeviceContext* device)
{
	VkFenceCreateInfo fenceCreateInfo = {};
	fenceCreateInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
	fenceCreateInfo.flags = 0;

	VkResult ret = vkCreateFence(device->vkDevice, &fenceCreateInfo, nullptr, &vkFence);
	if (ret != VK_SUCCESS)
	{
		std::printf("Error: Failed to create fence!\n");
		assert(0);
		return false;
	}

	return true;
}

bool Fence::Wait(DeviceContext* device)
{
	uint64_t timeoutns = (uint64_t)1e9;
	VkResult ret = vkWaitForFences(device->vkDevice, 1, &vkFence, VK_TRUE, timeoutns);

	vkDestroyFence(device->vkDevice, vkFence, nullptr);

	if (ret != VK_SUCCESS)
	{
		std::printf("Error: Failed to wait fence!\n");
		assert(0);
		return false;
	}

	return true;
}