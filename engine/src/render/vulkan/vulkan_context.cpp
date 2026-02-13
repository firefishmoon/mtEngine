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
    _enableDebug = true;
    _instance = VK_NULL_HANDLE;
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

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "mtEngine";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "mtEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_2;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    mtVector<const char*> requiredExtensions; 
    if (_enableDebug) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.push_back(glfwExtensions[i]);
    }

    MT_LOG_DEBUG("Required Vulkan Instance Extensions:");
    for (const char* ext : requiredExtensions) {
        MT_LOG_DEBUG("  {}", ext);
    }

    createInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Verify required extensions are available.
    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    mtVector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    for (const char* requiredExt : requiredExtensions) {
        b8 found = false;
        for (const auto& extProp : availableExtensions) {
            if (strcmp(requiredExt, extProp.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            MT_LOG_ERROR("Required Vulkan extension not available: {}", requiredExt);
            return false;
        }
    }

    // Validation layers
    mtVector<const char*> requiredLayers;
    if (_enableDebug) {
        requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
    }
    createInfo.enabledLayerCount = static_cast<u32>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    // Verify required layers are available.
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    mtVector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char* requiredLayer : requiredLayers) {
        b8 found = false;
        for (const auto& layerProp : availableLayers) {
            if (strcmp(requiredLayer, layerProp.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            MT_LOG_ERROR("Required Vulkan layer not available: {}", requiredLayer);
            return false;
        }
    }

    createInfo.enabledLayerCount = static_cast<u32>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }
    
    if (_enableDebug) {
    
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = &debugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            if (func(_instance, &createInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
                MT_LOG_ERROR("Failed to set up debug messenger");
                return false;
            }
        }

    }
    
    // create surface
    mtPlatformData* data = mtApplication::getInstance()->getPlatformData();
    VK_CHECK(glfwCreateWindowSurface(_instance, data->window, 0, &_surface));

    if (!_vulkanDevice.initialize(this)) {
        MT_LOG_ERROR("Failed to initialize Vulkan Device");
        return false;
    }

    // initialize swapchain
    _vulkanSwapChain.initialize(this, _width, _height);
    

    // semaphores & fences
    u32 MAX_FRAMES_IN_FLIGHT = _vulkanSwapChain.getImageCount();

    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = _vulkanDevice.getLogicalDevice();
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
    for (auto& cmdBuffer : _commandBuffers) {
        cmdBuffer.initialize(this, true);
    }


    createBuffers();

    MT_LOG_INFO("Vulkan Context Initialized");
    return true;
}

b8 mtVulkanContext::shutdown() {
    vkDeviceWaitIdle(_vulkanDevice.getLogicalDevice());

    _objectShader.shutdown();
    _indexBuffer.shutdown();
    _vertexBuffer.shutdown();

    for (auto& commandbuffer : _commandBuffers) {
        commandbuffer.shutdown();
    }


    VkDevice device = _vulkanDevice.getLogicalDevice();
    for (u32 i = 0; i < _vulkanSwapChain.getImageCount(); ++i) {
        vkDestroySemaphore(device, _imageAvailableSemaphores[i], 0);
        vkDestroySemaphore(device, _renderFinishedSemaphores[i], 0);
        vkDestroyFence(device, _inFlightFences[i], 0);
    }

    
    _vulkanSwapChain.shutdown();

    _vulkanDevice.shutdown();

    vkDestroySurfaceKHR(_instance, _surface, 0);

    if (_enableDebug) {
        auto func = (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
        func(_instance, _debugMessenger, 0);
    }

    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }

    MT_LOG_INFO("Vulkan Context Shutdown");
    return true;
}

b8 mtVulkanContext::createBuffers() {
    VkMemoryPropertyFlagBits memoryPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;

    const u64 vertexBufferSize = sizeof(glm::vec3) * 1024;
    if (!_vertexBuffer.initialize(this, 
                                  vertexBufferSize, 
                                  VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  memoryPropertyFlags, 
                                  true)) {
        MT_LOG_ERROR("Error creating vertex buffer.");
        return false;
    }

    MT_LOG_INFO("Vertex buffer created, size: {}", vertexBufferSize);

    const u64 indexBufferSize = sizeof(u32) * 1024;
    if (!_indexBuffer.initialize(this, 
                                  indexBufferSize, 
                                  VK_BUFFER_USAGE_INDEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                                  memoryPropertyFlags, 
                                  true)) {
        MT_LOG_ERROR("Error creating index buffer.");
        return false;
    }
    MT_LOG_INFO("Index buffer created, size: {}", indexBufferSize);
    return true; 
}
