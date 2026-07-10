//
//  Buffer.cpp
//
#include "Buffer.h"
#include <assert.h>
#include <string.h>

/*
================================================================================================

Buffer

================================================================================================
*/

/*
====================================================
Buffer::Buffer
====================================================
*/
Buffer::Buffer() : m_vkBuffer(VK_NULL_HANDLE), m_vkBufferMemory(VK_NULL_HANDLE), m_vkBufferSize(0), m_vkMemoryPropertyFlags(0), m_offset(0), m_mapped(nullptr)
{}

/*
====================================================
Buffer::Allocate
====================================================
*/
// bool Buffer::Allocate( DeviceContext * device, const void * data, int size, VkBufferUsageFlagBits usageFlags ) {
// 	VkResult result;

// 	m_vkBufferSize = size;

// 	VkBufferCreateInfo bufferInfo = {};
// 	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
// 	bufferInfo.size = m_vkBufferSize;
// 	bufferInfo.usage = usageFlags;
// 	bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

// 	result = vkCreateBuffer( device->m_vkDevice, &bufferInfo, nullptr, &m_vkBuffer );
// 	if ( VK_SUCCESS != result ) {
// 		printf( "ERROR: Failed to create buffer\n" );
// 		assert( 0 );
// 		return false;
// 	}

// 	VkMemoryRequirements memRequirements;
// 	vkGetBufferMemoryRequirements( device->m_vkDevice, m_vkBuffer, &memRequirements );

// 	m_vkMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT ;

// 	VkMemoryAllocateInfo allocInfo = {};
// 	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
// 	allocInfo.allocationSize = memRequirements.size;
// 	allocInfo.memoryTypeIndex = device->FindMemoryTypeIndex( memRequirements.memoryTypeBits, m_vkMemoryPropertyFlags );

// 	result = vkAllocateMemory( device->m_vkDevice, &allocInfo, nullptr, &m_vkBufferMemory );
// 	if ( VK_SUCCESS != result ) {
// 		printf( "ERROR: Failed to allocate buffer memory\n" );
// 		assert( 0 );
// 		return false;
// 	}

// 	if ( NULL != data ) {
// 		void * memory = MapBuffer( device );
// 		memcpy( memory, data, size );
// 		UnmapBuffer( device );
// 	}

// 	vkBindBufferMemory( device->m_vkDevice, m_vkBuffer, m_vkBufferMemory, 0 );
// 	return true;
// }
bool Buffer::Allocate(DeviceContext *device, const void *data, int size, VkBufferUsageFlagBits usageFlags)
{
    VkResult result;

    m_vkBufferSize = size;
    m_offset = 0;

    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.flags = 0;
    bufferInfo.size = m_vkBufferSize;
    bufferInfo.usage = usageFlags;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
    bufferInfo.queueFamilyIndexCount = 0;
    bufferInfo.pQueueFamilyIndices = nullptr;

    result = vkCreateBuffer(device->m_vkDevice, &bufferInfo, nullptr, &m_vkBuffer);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to create buffer\n");
        assert(0);
        return false;
    }

    VkMemoryRequirements memRequirements = {};
    vkGetBufferMemoryRequirements(device->m_vkDevice, m_vkBuffer, &memRequirements);

    m_vkMemoryPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    VkMemoryAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.pNext = nullptr;
    allocInfo.allocationSize = memRequirements.size;
    allocInfo.memoryTypeIndex = device->FindMemoryTypeIndex(memRequirements.memoryTypeBits, m_vkMemoryPropertyFlags);

    printf("[LegacyBufferAllocate] size=%d usage=0x%x bufferInfo.pNext=%p allocInfo.pNext=%p memoryType=%u memoryFlags=0x%x\n", size,
           static_cast<unsigned int>(usageFlags), bufferInfo.pNext, allocInfo.pNext, allocInfo.memoryTypeIndex,
           static_cast<unsigned int>(m_vkMemoryPropertyFlags));

    result = vkAllocateMemory(device->m_vkDevice, &allocInfo, nullptr, &m_vkBufferMemory);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to allocate buffer memory\n");
        assert(0);
        return false;
    }

    if (nullptr != data)
    {
        void *memory = MapBuffer(device);
        memcpy(memory, data, size);
        UnmapBuffer(device);
    }

    result = vkBindBufferMemory(device->m_vkDevice, m_vkBuffer, m_vkBufferMemory, 0);
    if (VK_SUCCESS != result)
    {
        printf("ERROR: Failed to bind buffer memory\n");
        assert(0);
        return false;
    }

    return true;
}

/*
====================================================
Buffer::Cleanup
====================================================
*/
// void Buffer::Cleanup(DeviceContext *device)
// {
//     vkDestroyBuffer(device->m_vkDevice, m_vkBuffer, nullptr);
//     vkFreeMemory(device->m_vkDevice, m_vkBufferMemory, nullptr);
// }
void Buffer::Cleanup(DeviceContext *device)
{
    if (device == nullptr)
    {
        return;
    }

    if (m_mapped != nullptr)
    {
        UnmapBuffer(device);
    }

    if (m_vkBuffer != VK_NULL_HANDLE)
    {
        vkDestroyBuffer(device->m_vkDevice, m_vkBuffer, nullptr);
        m_vkBuffer = VK_NULL_HANDLE;
    }

    if (m_vkBufferMemory != VK_NULL_HANDLE)
    {
        vkFreeMemory(device->m_vkDevice, m_vkBufferMemory, nullptr);
        m_vkBufferMemory = VK_NULL_HANDLE;
    }

    m_vkBufferSize = 0;
    m_vkMemoryPropertyFlags = 0;
    m_offset = 0;
    m_mapped = nullptr;
}

/*
====================================================
Buffer::MapBuffer
====================================================
*/
void *Buffer::MapBuffer(DeviceContext *device)
{
    if (device == nullptr || m_vkBufferMemory == VK_NULL_HANDLE)
    {
        return nullptr;
    }

    if (m_mapped != nullptr)
    {
        return m_mapped;
    }

    VkResult result = vkMapMemory(device->m_vkDevice, m_vkBufferMemory, m_offset, m_vkBufferSize, 0, &m_mapped);

    if (result != VK_SUCCESS)
    {
        m_mapped = nullptr;
        return nullptr;
    }

    return m_mapped;
}

/*
====================================================
Buffer::UnmapBuffer
====================================================
*/
void Buffer::UnmapBuffer(DeviceContext *device)
{
    if (device == nullptr || m_vkBufferMemory == VK_NULL_HANDLE || m_mapped == nullptr)
    {
        return;
    }

    vkUnmapMemory(device->m_vkDevice, m_vkBufferMemory);
    m_mapped = nullptr;
}

Buffer Buffer::CreateSectionView(DeviceContext *device, VkDeviceSize offset, VkDeviceSize size)
{
    Buffer section;

    section.m_vkBuffer = m_vkBuffer;
    section.m_vkBufferMemory = m_vkBufferMemory;
    section.m_vkBufferSize = size;
    section.m_vkMemoryPropertyFlags = m_vkMemoryPropertyFlags;
    section.m_offset = offset;
    section.m_mapped = nullptr;

    return section;
}
