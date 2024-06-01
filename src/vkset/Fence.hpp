#pragma once

#include <vulkan/vulkan.hpp>
#include "DeviceContext.hpp"

class Fence
{
public:
	Fence(DeviceContext* device) :deviceContext(device) { Create(device); }
	~Fence() { Wait(deviceContext); }

	VkFence vkFence;

private:
	bool Create(DeviceContext* device);
	bool Wait(DeviceContext* device);

	DeviceContext* deviceContext;
};