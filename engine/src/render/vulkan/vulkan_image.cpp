#include "vulkan_image.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "core/loggersystem.h"
#include <vulkan/vulkan_core.h>

mtVulkanImage::mtVulkanImage(mtVulkanDevice& mtVkDevice,
                u32 width,
                u32 height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                b8 createView,
                VkImageAspectFlags viewAspectFlags,
                u32 mipLevels) : _mtVkDevice(mtVkDevice) {
    if (!initialize(mtVkDevice, width, height, format, tiling, usage, properties, createView, viewAspectFlags, mipLevels)) {
        throw std::runtime_error("Failed to create Vulkan Image!");
    }
}


b8 mtVulkanImage::initialize(mtVulkanDevice& mtVkDevice,
                            u32 width,
                            u32 height,
                            VkFormat format,
                            VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            b8 createView,
                            VkImageAspectFlags viewAspectFlags,
                            u32 mipLevels) {
    if (mipLevels < 1) {
        MT_LOG_WARN("Mip levels must be at least 1. Setting to 1.");
        mipLevels = 1;
    }

    // Create image
    VkImageCreateInfo imageInfo{};
    imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
    imageInfo.imageType = VK_IMAGE_TYPE_2D;
    imageInfo.extent.width = width;
    imageInfo.extent.height = height;
    imageInfo.extent.depth = 1;
    imageInfo.mipLevels = mipLevels;
    imageInfo.arrayLayers = 1;
    imageInfo.format = format;
    imageInfo.tiling = tiling;
    imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    imageInfo.usage = usage;
    imageInfo.samples = VK_SAMPLE_COUNT_1_BIT;
    imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

    VkDevice device = _mtVkDevice.getLogicalDevice();

    if (vkCreateImage(device, &imageInfo, nullptr, &_image) != VK_SUCCESS) {
        return false;
    }

    // Allocate memory
    vkGetImageMemoryRequirements(device, _image, &_memoryRequirments);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = _memoryRequirments.size;
    allocInfo.memoryTypeIndex = _mtVkDevice.findMemoryType(_memoryRequirments.memoryTypeBits, properties);

    if (vkAllocateMemory(_mtVkDevice.getLogicalDevice(), &allocInfo, nullptr, &_memory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(device, _image, _memory, 0);

    _format = format;
    _width = width;
    _height = height;

    // Create image view if requested
    if (createView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = viewAspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &_imageView) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

void mtVulkanImage::shutdown() {
    VkDevice device = _mtVkDevice.getLogicalDevice();
    if (_imageView) {
        vkDestroyImageView(device, _imageView, 0);
        _imageView = 0;
    }
    if (_memory) {
        vkFreeMemory(device, _memory, 0);
        _memory = 0;
    }
    if (_image) {
        vkDestroyImage(device, _image, 0);
        _image = 0;
    }
}
