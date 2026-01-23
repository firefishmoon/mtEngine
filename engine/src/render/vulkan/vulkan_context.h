#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "defines.h"
#include "render/vulkan/vulkan_device.h"

class mtVulkanContext {
public:
    mtVulkanContext();
    ~mtVulkanContext();

    b8 initialize();
    b8 shutdown();

    VkInstance getInstance() const { return _instance; }
    const mtVulkanDevice& getVulkanDevice() const { return _vulkanDevice; }

    u32 _apiMajor;
    u32 _apiMinor;
    u32 _apiPatch;

    b8 _enableDebug;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
    mtVulkanDevice _vulkanDevice;
};
