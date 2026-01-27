#include "vulkan_swapchain.h"
#include "vulkan_context.h"
#include "core/loggersystem.h"

b8 mtVulkanSwapChain::initialize(mtVulkanContext* context, u32 width, u32 height) {
    _context = context;
    return create(width, height);
}

b8 mtVulkanSwapChain::create(u32 width, u32 height) {
    VkDevice device = _context->getVulkanDevice()->getDeviceContext()->_logicDevice;
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

    VkPresentModeKHR presentMode = VK_PRESENT_MODE_IMMEDIATE_KHR;
    for (u32 i = 0; i < _swapChainSupport._presentModeCount; i++) {
        if (_swapChainSupport._presentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR) {
            presentMode = VK_PRESENT_MODE_MAILBOX_KHR;
            break;
        }
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
    swapchainCreateInfo.presentMode = presentMode;
    swapchainCreateInfo.clipped = VK_TRUE;
    swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;

    VkResult result = vkCreateSwapchainKHR(
        device,
        &swapchainCreateInfo,
        nullptr,
        &_swapChainCtx._handler
    );
    if (result != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create swapchain! VkResult: {}", static_cast<int>(result));
        return false;
    }

    // images
    _swapChainCtx._imageCount = 0;
    vkGetSwapchainImagesKHR(
        device,
        _swapChainCtx._handler,
        &_swapChainCtx._imageCount,
        0 
    );
    _swapChainCtx._swapChainImages.resize(_swapChainCtx._imageCount);
    vkGetSwapchainImagesKHR(
        device,
        _swapChainCtx._handler,
        &_swapChainCtx._imageCount,
        _swapChainCtx._swapChainImages.data()
    );


    // image views
    _swapChainCtx._swapChainImageViews.resize(_swapChainCtx._imageCount);
    for (size_t i = 0; i < _swapChainCtx._imageCount; i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapChainCtx._swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _swapChainCtx._imageFormat.format;
        createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
        createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        createInfo.subresourceRange.baseMipLevel = 0;
        createInfo.subresourceRange.levelCount = 1;
        createInfo.subresourceRange.baseArrayLayer = 0;
        createInfo.subresourceRange.layerCount = 1;
        
        if (vkCreateImageView(
            device, 
            &createInfo, 
            nullptr, 
            &_swapChainCtx._swapChainImageViews[i]) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to create image view");
            return false;
        }
    }

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
