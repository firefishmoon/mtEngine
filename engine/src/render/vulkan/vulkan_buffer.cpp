#include "vulkan_buffer.h"
#include "render/vulkan/vulkan_error.h"
#include "vulkan_context.h"
#include "core/loggersystem.h"
#include "vulkan_command_buffer.h"
#include <vulkan/vulkan_core.h>

b8 mtVulkanBuffer::initialize(
        mtVulkanContext* context,
        u64 size,
        s32 usage,
        u32 memoryPropertyFlags,
        b8 bindOnCreate) {
    _context = context;
    _totalSize = size;
    _usage = usage;
    _memoryPropertyFlags = memoryPropertyFlags;

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = size;
    bufferInfo.usage = usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = context->getVulkanDevice()->getLogicalDevice();

    VK_CHECK(vkCreateBuffer(device, &bufferInfo, 0, &_handle));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, _handle, &requirements);
    _memoryIndex = context->getVulkanDevice()->findMemoryType(requirements.memoryTypeBits, _memoryPropertyFlags);
    if (_memoryIndex == -1) {
        MT_LOG_ERROR("Unable to create vulkan buffer because the required memory type index was not found");
        return false;
    }

    VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = (u32)_memoryIndex;

    VkResult result = vkAllocateMemory(device, &allocateInfo, 0, &_memory);
    if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Unable to create vulkan buffer because the required memory allocation failed. Error: {}", (s32)result);
        return false;
    }

    if (bindOnCreate) {
        bind(0);        
    }

    return true;
}

void mtVulkanBuffer::shutdown() {
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    if (_memory) {
        vkFreeMemory(device, _memory, 0);
        _memory = 0;
    }
    if (_handle) {
        vkDestroyBuffer(device, _handle, 0);
    }
    _totalSize = 0;
    _usage = (VkBufferUsageFlagBits)0;
    _isLocked = false;
}

b8 mtVulkanBuffer::resize(
        u64 newSize,
        VkQueue queue,
        VkCommandPool pool
    ) {

    VkBufferCreateInfo bufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    bufferInfo.size = newSize;
    bufferInfo.usage = _usage;
    bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    VkBuffer newBuffer;

    VK_CHECK(vkCreateBuffer(device, &bufferInfo, 0, &newBuffer));

    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(device, newBuffer, &requirements);

    VkMemoryAllocateInfo allocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    allocateInfo.allocationSize = requirements.size;
    allocateInfo.memoryTypeIndex = (u32)_memoryIndex;

    VkDeviceMemory newMemory;
    VkResult result = vkAllocateMemory(device, &allocateInfo, 0, &newMemory);
    if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Unable to create vulkan buffer because the required memory allocation failed. Error: {}", (s32)result);
        return false;
    }

    VK_CHECK(vkBindBufferMemory(device, newBuffer, newMemory, 0));

    copyTo(pool, 0, queue, 0, newBuffer, 0, _totalSize);

    vkDeviceWaitIdle(device);

    if (_memory) {
        vkFreeMemory(device, _memory, 0);
        _memory = 0;
    }
    if (_handle) {
        vkDestroyBuffer(device, _handle, 0);
    }
    _totalSize = newSize;
    _memory = newMemory;
    _handle = newBuffer;

    return true;
}

void mtVulkanBuffer::bind(u64 offset) {
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    VK_CHECK(vkBindBufferMemory(device, _handle, _memory, 0));
}

void* mtVulkanBuffer::lockMemory(u64 offset, u64 size, u32 flags) {
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    void* data;
    VK_CHECK(vkMapMemory(device, _memory, offset, size, flags, &data));
    return data;
}

void mtVulkanBuffer::unlockMemory() {
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    vkUnmapMemory(device, _memory);
}

void mtVulkanBuffer::loadData(u64 offset, u64 size, u32 flags, const void* data) {
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    void* dataPtr;
    VK_CHECK(vkMapMemory(device, _memory, offset, size, flags, &dataPtr));
    memcpy(dataPtr, data, size);
    vkUnmapMemory(device, _memory);
}

void mtVulkanBuffer::copyTo(
        VkCommandPool pool, 
        VkFence fence,
        VkQueue queue,
        u64 offset,
        VkBuffer dest,
        u64 destOffset,
        u64 size
    ) {
    vkQueueWaitIdle(queue);
    mtVulkanCommandBuffer tempCommandBuffer;
    tempCommandBuffer.initialize(_context, true);
    tempCommandBuffer.begin();

    VkBufferCopy copyRegion;
    copyRegion.srcOffset = offset;
    copyRegion.dstOffset = destOffset;
    copyRegion.size = size;

    vkCmdCopyBuffer(tempCommandBuffer.getHandle(), _handle, dest, 1, &copyRegion);

    tempCommandBuffer.end();
    tempCommandBuffer.shutdown();
}
