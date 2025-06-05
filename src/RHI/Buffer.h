//
//  Buffer.h
//
#pragma once
#include "DeviceContext.h"

/*
================================================================================================

Buffer

A general buffer

================================================================================================
*/

class Buffer {
public:
	Buffer();

	bool Allocate( DeviceContext * device, const void * data, int size, VkBufferUsageFlagBits usageFlags );
	void Cleanup( DeviceContext * device );
	void * MapBuffer( DeviceContext * device );
	void UnmapBuffer( DeviceContext * device );

	Buffer CreateSectionView(DeviceContext *device, VkDeviceSize offset, VkDeviceSize size);

	VkBuffer		m_vkBuffer;
	VkDeviceMemory	m_vkBufferMemory;
	VkDeviceSize	m_vkBufferSize;
	VkMemoryPropertyFlags m_vkMemoryPropertyFlags;
    VkDeviceSize m_offset = 0;
};