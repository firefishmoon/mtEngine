#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;

enum class mtVkCommandBufferState {
    MT_VK_COMMAND_BUFFER_STATE_READY,
    MT_VK_COMMAND_BUFFER_STATE_RECORDING,
    MT_VK_COMMAND_BUFFER_STATE_IN_RENDER_PASS,
    MT_VK_COMMAND_BUFFER_STATE_RECORDING_END,
    MT_VK_COMMAND_BUFFER_STATE_SUBMITTED,
    MT_VK_COMMAND_BUFFER_STATE_NOT_ALLOCATED,
};

struct mtVkCommandBufferContext {
    VkCommandBuffer _commandBuffer;
    mtVkCommandBufferState _state;
};

class mtVulkanDevice;

class mtVulkanCommandBuffer {
public:
    mtVulkanCommandBuffer(mtVulkanDevice& mtVkDevice, b8 isPrimary);
    ~mtVulkanCommandBuffer() {
        shutdown();
    }

    void begin();
    void end();

    inline VkCommandBuffer getHandle() const { return _handle; }

    // mtVkCommandBufferContext* getCommandBufferContext() { return &_commandBufferCtx; }
    // VkCommandBuffer beginSingleTimeCommands();
    // void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
protected:
    b8 initialize(b8 isPrimary);
    b8 shutdown();
    // access via public accessors instead of friendship

    mtVulkanContext* _context;
    // mtVkCommandBufferContext _commandBufferCtx;
    VkCommandBuffer _handle;
    mtVkCommandBufferState _state;

    mtVulkanDevice& _mtVkDevice;
};
