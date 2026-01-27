#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "core/std_wrapper.h"

struct mtVkSwapChainContext {
    VkSurfaceFormatKHR _imageFormat;
    VkSwapchainKHR _handler;
    u32 _imageCount;
    b8 _supportsBlitDest;
    b8 _supportsBlitSrc;
    // khandle swapchainColorTexture;
    u32 _imageIndex;
    mtVector<VkImage> _swapChainImages;
    mtVector<VkImageView> _swapChainImageViews;
};

struct mtVkSwapchainSupportInfo {
    /** @brief The surface capabilities. */
    VkSurfaceCapabilitiesKHR _capabilities;
    /** @brief The number of available surface formats. */
    u32 _formatCount;
    /** @brief An array of the available surface formats. */
    VkSurfaceFormatKHR* _formats;
    /** @brief The number of available presentation modes. */
    u32 _presentModeCount;
    /** @brief An array of available presentation modes. */
    VkPresentModeKHR* _presentModes;
};

class mtVulkanContext;

class mtVulkanSwapChain {
public:
    b8 initialize(mtVulkanContext* context, u32 width, u32 height);
    b8 create(u32 width, u32 height);
    b8 shutdown();

    b8 recreate(u32 width, u32 height);

    mtVulkanContext* getContext() { return _context; }
    mtVkSwapChainContext* getSwapChainContext() { return &_swapChainCtx; }
private:
    mtVulkanContext* _context;
    mtVkSwapChainContext _swapChainCtx;
    mtVkSwapchainSupportInfo _swapChainSupport;
};
