#include "vulkan_image.h"
#include "vulkan_context.h"
#include "vulkan_device.h"
#include "core/loggersystem.h"
#include <vulkan/vulkan_core.h>

mtVulkanImage::mtVulkanImage()
    : _pVulkanContext(nullptr) {
    _imageContext = {};
}

mtVulkanImage::~mtVulkanImage() {
    free();
}

b8 mtVulkanImage::create(mtVulkanContext* pVulkanContext,
                            u32 width,
                            u32 height,
                            VkFormat format,
                            VkImageTiling tiling,
                            VkImageUsageFlags usage,
                            VkMemoryPropertyFlags properties,
                            b8 createView,
                            u32 mipLevels) {
    _pVulkanContext = pVulkanContext;
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

    VkDevice device = _pVulkanContext->getVulkanDevice()->getDeviceContext()->_logicDevice;

    if (vkCreateImage(device, &imageInfo, nullptr, &_imageContext._image) != VK_SUCCESS) {
        return false;
    }

    // Allocate memory
    vkGetImageMemoryRequirements(device, _imageContext._image, &_imageContext._memoryRequirments);

    VkMemoryAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
    allocInfo.allocationSize = _imageContext._memoryRequirments.size;
    allocInfo.memoryTypeIndex = _pVulkanContext->getVulkanDevice()->findMemoryType(_imageContext._memoryRequirments.memoryTypeBits, properties);

    if (vkAllocateMemory(_pVulkanContext->getVulkanDevice()->getDeviceContext()->_logicDevice, &allocInfo, nullptr, &_imageContext._memory) != VK_SUCCESS) {
        return false;
    }

    vkBindImageMemory(device, _imageContext._image, _imageContext._memory, 0);

    _imageContext._format = format;
    _imageContext._width = width;
    _imageContext._height = height;

    // Create image view if requested
    if (createView) {
        VkImageViewCreateInfo viewInfo{};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = _imageContext._image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        if (vkCreateImageView(device, &viewInfo, nullptr, &_imageContext._imageView) != VK_SUCCESS) {
            return false;
        }
    }
    return true;
}

void mtVulkanImage::free() {
    VkDevice device = _pVulkanContext->getVulkanDevice()->getDeviceContext()->_logicDevice;
    if (_imageContext._imageView) {
        vkDestroyImageView(device, _imageContext._imageView, 0);
        _imageContext._imageView = 0;
    }
    if (_imageContext._memory) {
        vkFreeMemory(device, _imageContext._memory, 0);
        _imageContext._memory = 0;
    }
    if (_imageContext._image) {
        vkDestroyImage(device, _imageContext._image, 0);
        _imageContext._image = 0;
    }
}
