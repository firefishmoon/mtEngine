#include "vulkan_context.h"
#include "core/loggersystem.h"
#include "core/std_wrapper.h"

#include <glfw/glfw3.h>

static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT severity, VkDebugUtilsMessageTypeFlagsEXT type, const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
    if (severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT || severity == VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
        MT_LOG_ERROR("[VULKAN] validation layer: msg: {}", pCallbackData->pMessage);
    }
    return VK_FALSE;
}

mtVulkanContext::mtVulkanContext() {
    _context._apiMajor = 0;
    _context._apiMinor = 0;
    _context._apiPatch = 0;
    _context._enableDebug = true;
    _context._instance = VK_NULL_HANDLE;
}

mtVulkanContext::~mtVulkanContext() {
    shutdown();
}

b8 mtVulkanContext::initialize() {
    u32 apiVersion = 0;
    vkEnumerateInstanceVersion(&apiVersion);
    _context._apiMajor = VK_VERSION_MAJOR(apiVersion);
    _context._apiMinor = VK_VERSION_MINOR(apiVersion);
    _context._apiPatch = VK_VERSION_PATCH(apiVersion);

    MT_LOG_INFO("Vulkan API Version: {}.{}.{}", _context._apiMajor, _context._apiMinor, _context._apiPatch);

    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "My Vulkan App";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "mtEngine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;

    mtVector<const char*> requiredExtensions; 
    if (_context._enableDebug) {
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
    if (_context._enableDebug) {
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

    if (vkCreateInstance(&createInfo, nullptr, &_context._instance) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create Vulkan instance");
        return false;
    }
    
    if (_context._enableDebug) {
    
        VkDebugUtilsMessengerCreateInfoEXT createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = &debugCallback;

        auto func = (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(_context._instance, "vkCreateDebugUtilsMessengerEXT");
        if (func != nullptr) {
            if (func(_context._instance, &createInfo, nullptr, &_context._debugMessenger) != VK_SUCCESS) {
                MT_LOG_ERROR("Failed to set up debug messenger");
                return false;
            }
        }

    }
    
    if (!_vulkanDevice.initialize(this)) {
        MT_LOG_ERROR("Failed to initialize Vulkan Device");
        return false;
    }

    MT_LOG_INFO("Vulkan Context Initialized");
    return true;
}

b8 mtVulkanContext::shutdown() {
    _vulkanDevice.shutdown();
    if (_context._instance != VK_NULL_HANDLE) {
        vkDestroyInstance(_context._instance, nullptr);
        _context._instance = VK_NULL_HANDLE;
    }

    MT_LOG_INFO("Vulkan Context Shutdown");
    return true;
}
