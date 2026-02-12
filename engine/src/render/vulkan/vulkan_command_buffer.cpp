#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "core/loggersystem.h"

b8 mtVulkanCommandBuffer::initialize(mtVulkanContext* context, b8 isPrimary) {
    _context = context;
    VkCommandPool commandPool = _context->getVulkanDevice()->getGraphicsCommandPool();
    VkDevice device = _context->getVulkanDevice()->getLogicalDevice();
    VkCommandBufferAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    allocInfo.commandPool = commandPool;
    allocInfo.level = isPrimary ? VK_COMMAND_BUFFER_LEVEL_PRIMARY : VK_COMMAND_BUFFER_LEVEL_SECONDARY;
    allocInfo.commandBufferCount = 1;

    if (vkAllocateCommandBuffers(device, &allocInfo, &_handle) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to allocate command buffers");
        return false;
    }

    _state = mtVkCommandBufferState::MT_VK_COMMAND_BUFFER_STATE_READY;
    return true;
}

b8 mtVulkanCommandBuffer::shutdown() {
    vkFreeCommandBuffers(
        _context->getVulkanDevice()->getLogicalDevice(),
        _context->getVulkanDevice()->getGraphicsCommandPool(),
        1,
        &_handle
    );
    _handle = VK_NULL_HANDLE;
    _state = mtVkCommandBufferState::MT_VK_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    return true;
}

void mtVulkanCommandBuffer::begin() {
    vkResetCommandBuffer(_handle, 0);
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(_handle, &beginInfo) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to begin recording command buffer!");
    }
}

void mtVulkanCommandBuffer::end() {
    if (vkEndCommandBuffer(_handle) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to end recording command buffer!");
    }
}
