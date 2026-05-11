#pragma once

#include "defines.h"
#include "render/vulkan/vulkan_renderpass.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;

// struct mtVkDeviceContext {
//     VkPhysicalDevice _physicalDevice;
//     VkDevice _logicDevice;
//     VkQueue _graphicsQueue;
//     VkQueue _presentQueue;
//     VkQueue _transferQueue;
//     VkCommandPool _graphicsCommandPool;
//
//     u32 _graphicsFamilyIndex = -1;
//     u32 _presentFamilyIndex = -1;
//     u32 _transferFamilyIndex = -1;
// };

struct mtVkSwapchainSupportInfo;
class mtVulkanInstance;

class mtVulkanDevice {
public:
    mtVulkanDevice(mtVulkanInstance& instance, VkSurfaceKHR surface);

    ~mtVulkanDevice();

    // b8 initialize(mtVulkanContext* context);

    // b8 shutdown();

    void querySwapChainSupport(VkSurfaceKHR surface, mtVkSwapchainSupportInfo* outSupportInfo);

    inline mtVulkanContext* getContext() { return _context; }

    // Accessors to avoid friend-based direct member access
    inline VkCommandPool getGraphicsCommandPool() const { return _graphicsCommandPool; }
    inline VkDevice getLogicalDevice() const { return _logicDevice; }
    inline VkQueue getPresentQueue() const { return _presentQueue; }
    inline VkQueue getGraphicsQueue() const { return _graphicsQueue; }
    inline VkQueue getTransferQueue() const { return _transferQueue; }
    inline VkFormat getDepthFormat() const { return _depthFormat; }

    inline u32 getGraphicsFamilyIndex() const { return _graphicsFamilyIndex; }
    inline u32 getPresentFamilyIndex() const { return _presentFamilyIndex; }
    inline u32 getTransferFamilyIndex() const { return _transferFamilyIndex; }

    inline u32 getSwapChainWidth() const { return _swapChainWidth; }
    inline u32 getSwapChainHeight() const { return _swapChainHeight; }
    inline void setSwapChainDimensions(u32 w, u32 h) { _swapChainWidth = w; _swapChainHeight = h; }

    // mtVkDeviceContext* getDeviceContext() { return &_deviceContext; }
    b8 detectDepthFormat();

    s32 findMemoryType(u32 typeFilter, u32 propertyFlags);
private:
    b8 selectPhysicalDevice(mtVulkanInstance& instance, VkSurfaceKHR surface);
    // b8 isDeviceSuitable(VkPhysicalDevice device);
protected:
    // friend class mtVulkanContext;
    // mtVkDeviceContext _deviceContext;
    mtVulkanContext* _context;

    // Device context

    VkPhysicalDevice _physicalDevice;
    VkDevice _logicDevice;
    VkQueue _graphicsQueue;
    VkQueue _presentQueue;
    VkQueue _transferQueue;
    VkCommandPool _graphicsCommandPool;
    VkFormat _depthFormat;

    u32 _swapChainWidth = 0;
    u32 _swapChainHeight = 0;

    u32 _graphicsFamilyIndex = -1;
    u32 _presentFamilyIndex = -1;
    u32 _transferFamilyIndex = -1;
};