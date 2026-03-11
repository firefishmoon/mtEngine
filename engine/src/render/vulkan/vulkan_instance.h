#pragma once

#define VK_DEBUG
#include <vulkan/vulkan.h>

class mtVulkanInstance {
public:
    mtVulkanInstance();
    ~mtVulkanInstance();

    VkInstance getHandle() const { return _instance; }
private:
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* userData);

    VkInstance _instance;
    VkDebugUtilsMessengerEXT _debugMessenger;
};
