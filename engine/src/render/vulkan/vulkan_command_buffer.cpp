#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "core/loggersystem.h"

b8 mtVulkanCommandBuffer::initialize(mtVulkanContext* context, b8 isPrimary) {
    _context = context;
    VkCommandPool commandPool = _context->getVulkanDevice()->getDeviceContext()->_graphicsCommandPool;
    VkDevice device = _context->getVulkanDevice()->getDeviceContext()->_logicDevice;
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &_commandBufferCtx._commandBuffer) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to allocate command buffers");
        return false;
    }

    _commandBufferCtx._state = mtVkCommandBufferState::MT_VK_COMMAND_BUFFER_STATE_READY;
    return true;
}

b8 mtVulkanCommandBuffer::shutdown() {
    vkFreeCommandBuffers(
        _context->getVulkanDevice()->getDeviceContext()->_logicDevice,
        _context->getVulkanDevice()->getDeviceContext()->_graphicsCommandPool,
        1,
        &_commandBufferCtx._commandBuffer
    );
    _commandBufferCtx._commandBuffer = VK_NULL_HANDLE;
    _commandBufferCtx._state = mtVkCommandBufferState::MT_VK_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    return true;
}
