#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;

class mtVulkanDevice {
public:
    b8 initialize(mtVulkanContext* context);

    mtVulkanContext* getContext() const { return _context; }
// private:
    // b8 selectPhysicalDevice();
    // b8 isDeviceSuitable(VkPhysicalDevice device);
    VkPhysicalDevice _physicalDevice;
    VkDevice _logicDevice;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    mtVulkanContext* _context;
};
