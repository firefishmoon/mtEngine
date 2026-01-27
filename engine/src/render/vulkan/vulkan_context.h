#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

#include "defines.h"
#include "render/vulkan/vulkan_device.h"
#include "render/vulkan/vulkan_swapchain.h"
#include "render/vulkan/vulkan_command_buffer.h"
#include "core/std_wrapper.h"

struct mtVkContext {
    u32 _apiMajor;
    u32 _apiMinor;
    u32 _apiPatch;

    b8 _enableDebug;

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;

    VkSurfaceKHR _surface;
};

class mtVulkanContext {
public:
    mtVulkanContext();
    ~mtVulkanContext();

    b8 initialize();
    b8 shutdown();

    // VkInstance getInstance() const { return _instance; }
    mtVkContext* getVkContext() { return &_context; }
    mtVulkanDevice* getVulkanDevice() { return &_vulkanDevice; }
    mtVulkanSwapChain* getVulkanSwapChain() { return &_vulkanSwapChain; }
    mtVector<mtVulkanCommandBuffer>* getVulkanCommandBuffers() { return &_commandBuffers; }
private:
    mtVkContext _context;
    mtVulkanDevice _vulkanDevice;
    mtVulkanSwapChain _vulkanSwapChain;
    mtVector<mtVulkanCommandBuffer> _commandBuffers;
};
