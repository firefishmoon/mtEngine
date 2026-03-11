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
        MT_LOG_ERROR("Failed to create Vulkan Image!");
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


void mtVulkanImage::transitionLayout(
        VkCommandBuffer commandBuffer,
        VkImageLayout oldLayout,
        VkImageLayout newLayout
    ) {
    VkImageMemoryBarrier barrier{};
    barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
    barrier.oldLayout = oldLayout;
    barrier.newLayout = newLayout;
    barrier.srcQueueFamilyIndex = _mtVkDevice.getGraphicsFamilyIndex();
    barrier.dstQueueFamilyIndex = _mtVkDevice.getGraphicsFamilyIndex();
    barrier.image = _image;
    barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    barrier.subresourceRange.baseMipLevel = 0;
    barrier.subresourceRange.levelCount = 1;
    barrier.subresourceRange.baseArrayLayer = 0;
    barrier.subresourceRange.layerCount = 1;

    VkPipelineStageFlags sourceStage;
    VkPipelineStageFlags destinationStage;

    // Don't care about the old layout - transition to optimal layout (for the underlying implementation).
    if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
        barrier.srcAccessMask = 0;
        barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

        // Don't care what stage the pipeline is in at the start.
        sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;

        // Used for copying
        destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
    } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
        // Transitioning from a transfer destination layout to a shader-readonly layout.
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        // From a copying stage to...
        sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;

        // The fragment stage.
        destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
    } else {
        MT_LOG_ERROR("unsupported layout transition!");
        return;
    }

    vkCmdPipelineBarrier(
        commandBuffer,
        sourceStage, destinationStage,
        0,
        0, nullptr,
        0, nullptr,
        1, &barrier
    );
}

void mtVulkanImage::copyFromBuffer(
        VkCommandBuffer commandBuffer,
        VkBuffer buffer
    ) {
    VkBufferImageCopy region{};
    region.bufferOffset = 0;
    region.bufferRowLength = 0;
    region.bufferImageHeight = 0;
    region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
    region.imageSubresource.mipLevel = 0;
    region.imageSubresource.baseArrayLayer = 0;
    region.imageSubresource.layerCount = 1;
    region.imageOffset = {0, 0, 0};
    region.imageExtent = {
        _width,
        _height,
        1
    };

    vkCmdCopyBufferToImage(
        commandBuffer,
        buffer,
        _image,
        VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
        1,
        &region
    );
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
