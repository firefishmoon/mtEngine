#include "vulkan_instance.h"
#include "core/loggersystem.h"
#include "core/std_wrapper.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>
#include <glm/glm.hpp>

VKAPI_ATTR VkBool32 VKAPI_CALL
mtVulkanInstance::debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type,
                                const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT ||
        severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        MT_LOG_ERROR("[VULKAN] validation layer: msg: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

mtVulkanInstance::mtVulkanInstance() {
    _instance = VK_NULL_HANDLE;
    _debugMessenger = VK_NULL_HANDLE;

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

    mtVector<const char *> requiredExtensions;
#ifdef VK_DEBUG
    requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
#endif
    uint32_t glfwExtensionCount = 0;
    const char **glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);
    for (uint32_t i = 0; i < glfwExtensionCount; i++) {
        requiredExtensions.push_back(glfwExtensions[i]);
    }

    MT_LOG_DEBUG("Required Vulkan Instance Extensions:");
    for (const char *ext : requiredExtensions) {
        MT_LOG_DEBUG("  {}", ext);
    }

    createInfo.enabledExtensionCount = static_cast<u32>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    // Verify required extensions are available.
    u32 extensionCount = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);
    mtVector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, availableExtensions.data());
    for (const char *requiredExt : requiredExtensions) {
        b8 found = false;
        for (const auto &extProp : availableExtensions) {
            if (strcmp(requiredExt, extProp.extensionName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            MT_LOG_ERROR("Required Vulkan extension not available: {}", requiredExt);
            throw std::runtime_error("Missing required Vulkan extension");
        }
    }

    // Validation layers
    mtVector<const char *> requiredLayers;
#ifdef VK_DEBUG
    requiredLayers.push_back("VK_LAYER_KHRONOS_validation");
#endif
    createInfo.enabledLayerCount = static_cast<u32>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    // Verify required layers are available.
    u32 layerCount = 0;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);
    mtVector<VkLayerProperties> availableLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());
    for (const char *requiredLayer : requiredLayers) {
        b8 found = false;
        for (const auto &layerProp : availableLayers) {
            if (strcmp(requiredLayer, layerProp.layerName) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            MT_LOG_ERROR("Required Vulkan layer not available: {}", requiredLayer);
            throw std::runtime_error("Missing required Vulkan layer");
        }
    }

    createInfo.enabledLayerCount = static_cast<u32>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();

    if (vkCreateInstance(&createInfo, nullptr, &_instance) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create Vulkan instance");
        throw std::runtime_error("Failed to create Vulkan instance");
    }

    // if (_enableDebug) {
#ifdef VK_DEBUG
    VkDebugUtilsMessengerCreateInfoEXT messengerCreateInfo{};
    messengerCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
    messengerCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
    messengerCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
    messengerCreateInfo.pfnUserCallback = &debugCallback;

    auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        if (func(_instance, &messengerCreateInfo, nullptr, &_debugMessenger) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to set up debug messenger");
            throw std::runtime_error("Failed to set up Vulkan debug messenger");
        }
    }
#endif
}

mtVulkanInstance::~mtVulkanInstance() {
    if (_debugMessenger != VK_NULL_HANDLE) {
        auto func =
            (PFN_vkDestroyDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_instance, "vkDestroyDebugUtilsMessengerEXT");
        if (func != nullptr) {
            func(_instance, _debugMessenger, nullptr);
            _debugMessenger = VK_NULL_HANDLE;
        }
    }

    if (_instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_instance, nullptr);
        _instance = VK_NULL_HANDLE;
    }
}
