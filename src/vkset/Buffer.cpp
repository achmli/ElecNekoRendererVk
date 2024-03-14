#include "Buffer.h"

#include <assert.h>
#include <string.h>

Buffer::Buffer()
	:m_vkBufferSize(0) 
{}

bool Buffer::Allocate(DeviceContext* device, const void* data, int size, VkBufferUsageFlagBits usageFlags)
{
	VkResult result;

	m_vkBufferSize = size;

	VkBufferCreateInfo bufferInfo= {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.size = m_vkBufferSize;
	bufferInfo.usage = usageFlags;
	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	result = vkCreateBuffer(device->m_vkDevice, &bufferInfo, nullptr, &m_vkBuffer);
	if (result != VK_SUCCESS)
	{
		printf("ERROR: Failed to Create Buffer!\n");
		assert(0);
		return false;
	}

	VkMemoryRequirements memRequirements;
	vkGetBufferMemoryRequirements(device->m_vkDevice, m_vkBuffer, &memRequirements);
	/**
	* VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT2 ¨C 
	* this is referring to CPU memory that is directly visible from GPU;
	* reads from this memory go over PCI-express bus. 
	* In absence of the previous memory type, this generally speaking should be the choice for uniform buffers or dynamic vertex/index buffers, 
	* and also should be used to store staging buffers that are used to populate static resources allocated with VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT with data.
	*/
	m_vkMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memRequirements.size;
	allocInfo.memoryTypeIndex = device->FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_vkMemoryPropertyFlags);

	result = vkAllocateMemory(device->m_vkDevice, &allocInfo, nullptr, &m_vkDeviceMemory);
	if (result != VK_SUCCESS)
	{
		printf("ERROR: Failed to allocate memory!\n");
		assert(0);
		return false;
	}

	if (data != NULL)
	{
		void* memory = MapBuffer(device);
		memcpy(memory, data, size);
		UnmapBuffer(device);
	}

	vkBindBufferMemory(device->m_vkDevice, m_vkBuffer, m_vkDeviceMemory, 0);
	return true;
}

void Buffer::CleanUp(DeviceContext* device)
{
	vkDestroyBuffer(device->m_vkDevice, m_vkBuffer, nullptr);
	vkFreeMemory(device->m_vkDevice, m_vkDeviceMemory, nullptr);
}

void* Buffer::MapBuffer(DeviceContext* device)
{
	void* mapped_ptr = NULL;
	vkMapMemory(device->m_vkDevice, m_vkDeviceMemory, 0, m_vkBufferSize, 0, &mapped_ptr);
	return mapped_ptr;
}

void Buffer::UnmapBuffer(DeviceContext* device)
{
	vkUnmapMemory(device->m_vkDevice, m_vkDeviceMemory);
}