#include "vulkan_context.h"
#include "vulkan_error.h"
#include "core/loggersystem.h"
#include "core/std_wrapper.h"
#include "core/application.h"
#include "core/platform.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glm/glm.hpp>
#include "core/application.h"

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT || severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        MT_LOG_ERROR("[VULKAN] validation layer: msg: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

mtVulkanContext::mtVulkanContext() {
    _apiMajor = 0;
    _apiMinor = 0;
    _apiPatch = 0;
    // _instance = VK_NULL_HANDLE;
}

mtVulkanContext::~mtVulkanContext() {
    shutdown();
}

b8 mtVulkanContext::initialize(u32 width, u32 height) {
    _width = width;
    _height = height;

    u32 apiVersion = 0;
    vkEnumerateInstanceVersion(&apiVersion);
    _apiMajor = VK_VERSION_MAJOR(apiVersion);
    _apiMinor = VK_VERSION_MINOR(apiVersion);
    _apiPatch = VK_VERSION_PATCH(apiVersion);

    MT_LOG_INFO("Vulkan API Version: {}.{}.{}", _apiMajor, _apiMinor, _apiPatch);

    _instance = std::make_unique<mtVulkanInstance>();

    _surface = std::make_unique<mtVulkanSurface>(*_instance);

    _vulkanDevice = std::make_unique<mtVulkanDevice>(*_instance, _surface->getHandle());

    _vulkanSwapChain = std::make_unique<mtVulkanSwapChain>(*_vulkanDevice, _surface->getHandle(), _width, _height);
    // _vulkanSwapChain.initialize(this, _width, _height);

    _mainRenderPass = std::make_unique<mtVulkanRenderPass>(
        *_vulkanDevice,
        *_vulkanSwapChain,
        0.0f, 0.0f, (f32)_width, (f32)_height,
        0.0f, 0.0f, 0.2f, 1.0f,
        1.0f, 0.0f);

    // semaphores & fences
    u32 MAX_FRAMES_IN_FLIGHT = _vulkanSwapChain->getImageCount();

    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = _vulkanDevice->getLogicalDevice();
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to create sync objects");
            return false;
        }
    }

    // command buffer
    _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    for (int i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) {
        // cmdBuffer.initialize(this, true);
        _commandBuffers[i] = std::make_unique<mtVulkanCommandBuffer>(*_vulkanDevice, true);
    }


    createBuffers();

    MT_LOG_INFO("Vulkan Context Initialized");
    return true;
}

b8 mtVulkanContext::shutdown() {
    vkDeviceWaitIdle(_vulkanDevice->getLogicalDevice());

    _objectShader.shutdown();
    // _indexBuffer.shutdown();
    // _vertexBuffer.shutdown();

    // for (auto& commandbuffer : _commandBuffers) {
    //     commandbuffer.shutdown();
    // }


    VkDevice device = _vulkanDevice->getLogicalDevice();
    for (u32 i = 0; i < _vulkanSwapChain->getImageCount(); ++i) {
        vkDestroySemaphore(device, _imageAvailableSemaphores[i], 0);
        vkDestroySemaphore(device, _renderFinishedSemaphores[i], 0);
        vkDestroyFence(device, _inFlightFences[i], 0);
    }


    // _vulkanSwapChain.shutdown();

    // _vulkanDevice.shutdown();

    // vkDestroySurfaceKHR(_instance->getHandle(), _surface, 0);

    MT_LOG_INFO("Vulkan Context Shutdown");
    return true;
}

b8 mtVulkanContext::createBuffers() {
    VkMemoryPropertyFlagBits memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertexBufferSize = sizeof(glm::vec3) * 1024;
    // if (!_vertexBuffer.initialize(this,
    //                               vertexBufferSize,
    //                               VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //                               memoryPropertyFlags,
    //                               true)) {
    //     MT_LOG_ERROR("Error creating vertex buffer.");
    //     return false;
    // }
    _vertexBuffer = std::make_unique<mtVulkanBuffer>(
        *_vulkanDevice,
        vertexBufferSize,
        VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        memoryPropertyFlags,
        true
    );

    MT_LOG_INFO("Vertex buffer created, size: {}", vertexBufferSize);

    const u64 indexBufferSize = sizeof(u32) * 1024;
    // if (!_indexBuffer.initialize(this,
    //                               indexBufferSize,
    //                               VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
    //                               memoryPropertyFlags,
    //                               true)) {
    //     MT_LOG_ERROR("Error creating index buffer.");
    //     return false;
    // }
    _indexBuffer = std::make_unique<mtVulkanBuffer>(
        *_vulkanDevice,
        indexBufferSize,
        VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
        memoryPropertyFlags,
        true
    );
    MT_LOG_INFO("Index buffer created, size: {}", indexBufferSize);
    return true;
}
