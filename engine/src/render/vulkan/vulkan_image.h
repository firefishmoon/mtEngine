#pragma once

#include <vulkan/vulkan.h>
#include "defines.h"


class mtVulkanContext;

class mtVulkanImage {
public:
    mtVulkanImage();
    ~mtVulkanImage();

    b8 create(mtVulkanContext* pVulkanContext,
                u32 width,
                u32 height,
                VkFormat format,
                VkImageTiling tiling,
                VkImageUsageFlags usage,
                VkMemoryPropertyFlags properties,
                b8 createView,
                u32 mipLevels);

    void free();


    // mtVkImageContext* getImageContext() { return &_imageContext; }
private:
    mtVulkanContext* _pVulkanContext;
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
