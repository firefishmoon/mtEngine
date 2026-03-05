#pragma once

#include <vulkan/vulkan.h>
#include "defines.h"


class mtVulkanDevice;

class mtVulkanImage {
public:
    mtVulkanImage(mtVulkanDevice& mtVkDevice,
                u32 width,
                u32 height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                b8 createView,
                VkImageAspectFlags viewAspectFlags,
                u32 mipLevels);

    ~mtVulkanImage() {
        shutdown();
    }

    inline VkImageView getImageView() const { return _imageView; }


    // mtVkImageContext* getImageContext() { return &_imageContext; }
protected:
    b8 initialize(mtVulkanDevice& mtVkDevice,
                u32 width,
                u32 height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                b8 createView,
                VkImageAspectFlags viewAspectFlags,
                u32 mipLevels);
    void shutdown();
    // access via public accessors instead of friendship

    // mtVulkanContext* _pVulkanContext;
    mtVulkanDevice& _mtVkDevice;
    // mtVkImageContext _imageContext;
    VkImage _image;
    VkDeviceMemory _memory;
    VkImageView _imageView;
    VkMemoryRequirements _memoryRequirments;
    VkFormat _format;
    u32 _width;
    u32 _height;
    u32 _mipLevels;
};
