#include "vulkan_swapchain.h"
#include "vulkan_context.h"
#include "core/loggersystem.h"

b8 mtVulkanSwapChain::initialize(mtVulkanContext* context, u32 width, u32 height) {
    _context = context;
    return create(width, height);
}

b8 mtVulkanSwapChain::create(u32 width, u32 height) {
    VkExtent2D swapchainExtent = {width, height};

    _context->getVulkanDevice()->querySwapChainSupport(
        _context->getVkContext()->_surface, 
        &_swapChainSupport
    );

    b8 found = false;
    for (u32 i = 0; i < _swapChainSupport._formatCount; i++) {
        VkSurfaceFormatKHR& currentFormat = _swapChainSupport._formats[i];
        if (currentFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _swapChainCtx._imageFormat = currentFormat;
            found = true;
            break;
        }
    }

    if (!found) {
        MT_LOG_ERROR("Failed to find suitable surface format for swapchain!");
        return false;
    }
    
    VkExtent2D min = _swapChainSupport._capabilities.minImageExtent;
    VkExtent2D max = _swapChainSupport._capabilities.maxImageExtent;
    swapchainExtent.width = MT_CLAMP(swapchainExtent.width, min.width, max.width);
    swapchainExtent.height = MT_CLAMP(swapchainExtent.height, min.height, max.height);

    u32 desiredImageCount = _swapChainSupport._capabilities.minImageCount + 1;
    if (_swapChainSupport._capabilities.maxImageCount > 0 && 
        desiredImageCount > _swapChainSupport._capabilities.maxImageCount) {
        desiredImageCount = _swapChainSupport._capabilities.maxImageCount;
    }

    VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
    swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
    swapchainCreateInfo.surface = _context->getVkContext()->_surface;
    swapchainCreateInfo.minImageCount = desiredImageCount;
    swapchainCreateInfo.imageFormat = _swapChainCtx._imageFormat.format;
    swapchainCreateInfo.imageColorSpace = _swapChainCtx._imageFormat.colorSpace;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (_context->getVulkanDevice()->getDeviceContext()->_graphicsFamilyIndex != 
        _context->getVulkanDevice()->getDeviceContext()->_presentFamilyIndex) {
        u32 queueFamilyIndices[] = {
            _context->getVulkanDevice()->getDeviceContext()->_graphicsFamilyIndex,
            _context->getVulkanDevice()->getDeviceContext()->_presentFamilyIndex
        };
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
        swapchainCreateInfo.queueFamilyIndexCount = 2;
        swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    } else {
        swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        swapchainCreateInfo.queueFamilyIndexCount = 0;
        swapchainCreateInfo.pQueueFamilyIndices = nullptr;
    }
    swapchainCreateInfo.preTransform = _swapChainSupport._capabilities.currentTransform;
    swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
    swapchainCreateInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR; // Guaranteed to be available
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
        _context->getVulkanDevice()->getDeviceContext()->_logicDevice,
        &swapchainCreateInfo,
        nullptr,
        &_swapChainCtx._handler
    );
    if (result != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create swapchain! VkResult: {}", static_cast<int>(result));
        return false;
    }

    _swapChainCtx._imageCount = 0;
    vkGetSwapchainImagesKHR(
        _context->getVulkanDevice()->getDeviceContext()->_logicDevice,
        _swapChainCtx._handler,
        &_swapChainCtx._imageCount,
        nullptr
    );

    MT_LOG_INFO("Swapchain created with {} images.", _swapChainCtx._imageCount);
    return true;
}

b8 mtVulkanSwapChain::shutdown() {
    vkDeviceWaitIdle(_context->getVulkanDevice()->getDeviceContext()->_logicDevice);
    if (_swapChainCtx._handler != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            _context->getVulkanDevice()->getDeviceContext()->_logicDevice,
            _swapChainCtx._handler,
            nullptr
        );
        _swapChainCtx._handler = VK_NULL_HANDLE;
    }
    return true;
}

b8 mtVulkanSwapChain::recreate(u32 width, u32 height) {
    if (!shutdown()) {
        MT_LOG_ERROR("Failed to shutdown swapchain during recreation!");
        return false;
    }
    if (!create(width, height)) {
        MT_LOG_ERROR("Failed to recreate swapchain!");
        return false;
    }
    return true;
}
