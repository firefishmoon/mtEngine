#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;

class mtVulkanBuffer {
public:
    mtVulkanBuffer():_context(0), 
        _totalSize(0),
        _memory(0),
        _isLocked(false),
        _handle(0) {}
    b8 initialize(
        mtVulkanContext* context,
        u64 size,
        s32 usage,
        u32 memoryPropertyFlags,
        b8 bindOnCreate
    );
    void shutdown();

    b8 resize(
        u64 newSize,
        VkQueue queue,
        VkCommandPool pool
    );

    void bind(u64 offset);
    void* lockMemory(u64 offset, u64 size, u32 flags);
    void unlockMemory();
    void loadData(u64 offset, u64 size, u32 flags, const void* data);
    void copyTo(
        VkCommandPool pool, 
        VkFence fence,
        VkQueue queue,
        u64 offset,
        VkBuffer dest,
        u64 destOffset,
        u64 size
    );

    inline VkBuffer getHandle() { return _handle; }
private:
    mtVulkanContext* _context;
    u64 _totalSize;
    VkBuffer _handle;
    s32 _usage;
    b8 _isLocked;
    VkDeviceMemory _memory;
    s32 _memoryIndex;
    u32 _memoryPropertyFlags;
};
