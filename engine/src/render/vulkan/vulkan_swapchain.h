#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "core/std_wrapper.h"
#include "vulkan_image.h"
#include "vulkan_framebuffer.h"

// struct mtVkSwapChainContext {
//
// };

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
class mtVulkanDevice;

class mtVulkanSwapChain {
public:
    ~mtVulkanSwapChain();
    b8 initialize(mtVulkanDevice& mtVkdevice, u32 width, u32 height);
    b8 create(u32 width, u32 height);
    b8 shutdown();

    b8 recreate(u32 width, u32 height);

    b8 represent();

    inline mtVulkanContext* getContext() { return _context; }
    inline VkSurfaceFormatKHR getImageFormat() const { return _imageFormat; }
    inline u32 getImageCount() const { return _imageCount; }
    inline mtVector<mtVulkanFrameBuffer>& getFramebuffers() { return _framebuffers; }
    inline VkSwapchainKHR getHandle() const { return _handle; }
    inline u32 getImageIndex() const { return _imageIndex; }
    inline void setImageIndex(u32 idx) { _imageIndex = idx; }
    inline mtVector<VkImageView>& getSwapChainImageViews() { return _swapChainImageViews; }
    inline mtVulkanImage& getDepthAttachment() { return _depthAttachment; }
    // mtVkSwapChainContext* getSwapChainContext() { return &_swapChainCtx; }
protected:
    // access via public accessors instead of friendship

    mtVulkanContext* _context;

    // mtVkSwapChainContext _swapChainCtx;
    mtVkSwapchainSupportInfo _swapChainSupport;

    VkSurfaceFormatKHR _imageFormat;
    VkSwapchainKHR _handle;
    u32 _imageCount;
    b8 _supportsBlitDest;
    b8 _supportsBlitSrc;
    // khandle swapchainColorTexture;
    u32 _imageIndex;
    mtVector<VkImage> _swapChainImages;
    mtVector<VkImageView> _swapChainImageViews;
    mtVector<mtVulkanFrameBuffer> _framebuffers;

    mtVulkanImage _depthAttachment;

};
