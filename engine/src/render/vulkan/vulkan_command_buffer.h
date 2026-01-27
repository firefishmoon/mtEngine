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

class mtVulkanCommandBuffer {
public:
    b8 initialize(mtVulkanContext* context, b8 isPrimary);
    b8 shutdown();

    mtVkCommandBufferContext* getCommandBufferContext() { return &_commandBufferCtx; }
    // VkCommandBuffer beginSingleTimeCommands();
    // void endSingleTimeCommands(VkDevice device, VkQueue graphicsQueue, VkCommandBuffer commandBuffer);
private:
    mtVulkanContext* _context;
    mtVkCommandBufferContext _commandBufferCtx;
};
