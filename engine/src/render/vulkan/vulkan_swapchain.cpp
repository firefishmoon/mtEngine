#include "vulkan_swapchain.h"
#include "vulkan_device.h"
#include "vulkan_context.h"
#include "core/loggersystem.h"

mtVulkanSwapChain::mtVulkanSwapChain(mtVulkanDevice& mtVkDevice, VkSurfaceKHR surface, u32 width, u32 height) : _mtVkDevice(mtVkDevice), _surface(surface) {
    if (!create(width, height)) {
        throw std::runtime_error("Failed to create Vulkan SwapChain!");
    }
}

// b8 mtVulkanSwapChain::initialize(mtVulkanContext* context, u32 width, u32 height) {
//     _context = context;
//     return create(width, height);
// }

mtVulkanSwapChain::~mtVulkanSwapChain() {
    shutdown();
}

b8 mtVulkanSwapChain::create(u32 width, u32 height) {
    VkDevice device = _mtVkDevice.getLogicalDevice();
    VkExtent2D swapchainExtent = {width, height};

    _mtVkDevice.querySwapChainSupport(
        _surface,
        &_swapChainSupport
    );

    b8 found = false;
    for (u32 i = 0; i < _swapChainSupport._formatCount; i++) {
        VkSurfaceFormatKHR& currentFormat = _swapChainSupport._formats[i];
        if (currentFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
            currentFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
            _imageFormat = currentFormat;
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
    swapchainCreateInfo.surface = _surface;
    swapchainCreateInfo.minImageCount = desiredImageCount;
    swapchainCreateInfo.imageFormat = _imageFormat.format;
    swapchainCreateInfo.imageExtent = swapchainExtent;
    swapchainCreateInfo.imageArrayLayers = 1;
    swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

    if (_mtVkDevice.getGraphicsFamilyIndex() !=
        _mtVkDevice.getPresentFamilyIndex()) {
        u32 queueFamilyIndices[] = {
            _mtVkDevice.getGraphicsFamilyIndex(),
            _mtVkDevice.getPresentFamilyIndex()
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
        &_handle
    );
    if (result != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create swapchain! VkResult: {}", static_cast<int>(result));
        return false;
    }

    // images
    _imageCount = 0;
    vkGetSwapchainImagesKHR(
        device,
        _handle,
        &_imageCount,
        0
    );
    _swapChainImages.resize(_imageCount);
    vkGetSwapchainImagesKHR(
        device,
        _handle,
        &_imageCount,
        _swapChainImages.data()
    );

    // image views
    _swapChainImageViews.resize(_imageCount);
    for (size_t i = 0; i < _imageCount; i++) {
        VkImageViewCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        createInfo.image = _swapChainImages[i];
        createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        createInfo.format = _imageFormat.format;
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
            &_swapChainImageViews[i]) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to create image view");
            return false;
        }
    }

    if (!_mtVkDevice.detectDepthFormat()) {
        MT_LOG_FATAL("VulkanDevice detectDepthFormat Failed");
        return false;
    }

    // _depthAttachment.initialize(
    _depthAttachment = std::make_unique<mtVulkanImage>(
        _mtVkDevice,
        width,
        height,
        _mtVkDevice.getDepthFormat(),
        VK_IMAGE_TILING_OPTIMAL,
        VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
        true,
        VK_IMAGE_ASPECT_DEPTH_BIT,
        1);

    // framebuffers
    _framebuffers.resize(_imageCount);

    MT_LOG_INFO("Swapchain created with {} images.", _imageCount);
    return true;
}

b8 mtVulkanSwapChain::shutdown() {
    VkDevice device = _mtVkDevice.getLogicalDevice();
    vkDeviceWaitIdle(device);

    for (size_t i = 0; i < _imageCount; i++) {
        vkDestroyImageView(device, _swapChainImageViews[i], 0);
        _framebuffers[i].shutdown();
    }

    // _depthAttachment.shutdown();

    if (_handle != VK_NULL_HANDLE) {
        vkDestroySwapchainKHR(
            device,
            _handle,
            nullptr
        );
        _handle = VK_NULL_HANDLE;
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

b8 mtVulkanSwapChain::represent(mtVulkanContext* context) {
    VkSwapchainKHR swapChain = _handle;

    VkSemaphore signalSemaphores[] = { context->getRenderFinishedSemaphores()[ context->getCurrentFrame() ] };
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;

    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &_imageIndex;

    VkResult result = vkQueuePresentKHR(_mtVkDevice.getPresentQueue(), &presentInfo);

    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR) {
        // _framebufferResized = false;
        // return recreateSwapChain();
        return recreate(context->getWidth(), context->getHeight());
    } else if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to present swap chain image!");
        return false;
    }

    context->setCurrentFrame((context->getCurrentFrame() + 1) % _imageCount);
    return true;
}
