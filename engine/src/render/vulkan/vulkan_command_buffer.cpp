#include "vulkan_command_buffer.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "core/loggersystem.h"

mtVulkanCommandBuffer::mtVulkanCommandBuffer(mtVulkanDevice& mtVkDevice, b8 isPrimary) : _mtVkDevice(mtVkDevice) {
    _handle = VK_NULL_HANDLE;
    _state = mtVkCommandBufferState::MT_VK_COMMAND_BUFFER_STATE_NOT_ALLOCATED;
    if (!initialize(isPrimary)) {
        MT_LOG_ERROR("Failed to initialize Vulkan command buffer");
        throw std::runtime_error("Failed to initialize Vulkan command buffer");
    }
}

b8 mtVulkanCommandBuffer::initialize(b8 isPrimary) {
    // _context = context;
    VkCommandPool commandPool = _mtVkDevice.getGraphicsCommandPool();
    VkDevice device = _mtVkDevice.getLogicalDevice();
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
        _mtVkDevice.getLogicalDevice(),
        _mtVkDevice.getGraphicsCommandPool(),
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
