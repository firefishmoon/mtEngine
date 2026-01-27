#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;

struct mtVkDeviceContext {
    VkPhysicalDevice _physicalDevice;
    VkDevice _logicDevice;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkQueue _transferQueue;
    VkCommandPool _graphicsCommandPool;

    u32 _graphicsFamilyIndex = -1;
    u32 _presentFamilyIndex = -1;
    u32 _transferFamilyIndex = -1;
};

struct mtVkSwapchainSupportInfo;

class mtVulkanDevice {
public:
    b8 initialize(mtVulkanContext* context);

    b8 shutdown();

    void querySwapChainSupport(VkSurfaceKHR surface, mtVkSwapchainSupportInfo* outSupportInfo);

    mtVulkanContext* getContext() { return _context; }

    mtVkDeviceContext* getDeviceContext() { return &_deviceContext; }

    s32 findMemoryType(u32 typeFilter, u32 propertyFlags);
// private:
    // b8 selectPhysicalDevice();
    // b8 isDeviceSuitable(VkPhysicalDevice device);
private:
    mtVkDeviceContext _deviceContext;
    mtVulkanContext* _context;
};
