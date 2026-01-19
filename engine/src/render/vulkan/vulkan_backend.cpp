#include <stdexcept>
#include <algorithm>
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#define VK_USE_PLATFORM_WIN32_KHR 
#include "vulkan_backend.h"
#include "core/loggersystem.h"
#include "core/application.h"
#include "core/platform.h"
#include <windows.h>

constexpr bool enableValidationLayers = true;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

b8 mtVulkanBackend::initialize() {
    // Initialize Vulkan-specific resources and state
    MT_LOG_INFO("Initializing Vulkan Backend");

    vk::ApplicationInfo appInfo{};
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    

    // Get the required layers
    std::vector<const char*> requiredLayers;
    if (enableValidationLayers) {
        requiredLayers.assign(validationLayers.begin(), validationLayers.end());
    }

    // Check if the required layers are supported by the Vulkan implementation.
    auto layerProperties = _context.enumerateInstanceLayerProperties();
    for (auto const& requiredLayer : requiredLayers) {
        bool layerFound = false;
        for (const auto& layerProperty : layerProperties) {
            if (strcmp(requiredLayer, layerProperty.layerName) == 0) {
                layerFound = true;
                break;
            }
        }
        if (!layerFound) {
            MT_LOG_ERROR("Required validation layer not available: %s", requiredLayer);
            return false; // Or throw, but since it's b8, return false
        }
    }

    // Get the required instance extensions from GLFW.
    uint32_t glfwExtensionCount = 0;
    const char** glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

    std::vector<const char*> requiredExtensions(glfwExtensions, glfwExtensions + glfwExtensionCount);
    if (enableValidationLayers) {
        requiredExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
    }

    // Check if the required extensions are supported
    auto extensionProperties = _context.enumerateInstanceExtensionProperties();
    for (auto const& requiredExtension : requiredExtensions) {
        bool extensionFound = false;
        for (const auto& extensionProperty : extensionProperties) {
            if (strcmp(requiredExtension, extensionProperty.extensionName) == 0) {
                extensionFound = true;
                break;
            }
        }
        if (!extensionFound) {
            MT_LOG_ERROR("Required extension not available: %s", requiredExtension);
            return false;
        }
    }

    vk::InstanceCreateInfo createInfo{};
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledLayerCount = static_cast<uint32_t>(requiredLayers.size());
    createInfo.ppEnabledLayerNames = requiredLayers.data();
    createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensions.size());
    createInfo.ppEnabledExtensionNames = requiredExtensions.data();

    try {
        _instance = vk::raii::Instance(_context, createInfo);
    } catch (const vk::LayerNotPresentError& e) {
        MT_LOG_ERROR("Failed to create Vulkan instance: %s", e.what());
        return false;
    }

    if (!setupDebugMessenger()) {
        MT_LOG_ERROR("Failed to set up debug messenger!");
        return false;
    }
    if (!createSurface()) {
        MT_LOG_ERROR("Failed to create surface!");
        return false;
    }

    if (!pickPhysicalDevice()) {
        MT_LOG_ERROR("Failed to pick physical device!");
        return false;
    }

    if (!createLogicalDevice()) {
        MT_LOG_ERROR("Failed to create logical device!");
        return false;
    }
    
    MT_LOG_INFO("Vulkan Backend Initialized");

    return true;
}
b8 mtVulkanBackend::shutdown() {
    // Clean up Vulkan-specific resources and state
    vkDestroySurfaceKHR(static_cast<VkInstance>(*_instance), _surface, nullptr);
    vkDestroyDevice(_device, nullptr);
    return true;
}

static VKAPI_ATTR vk::Bool32 VKAPI_CALL debugCallback(vk::DebugUtilsMessageSeverityFlagBitsEXT severity, vk::DebugUtilsMessageTypeFlagsEXT type, const vk::DebugUtilsMessengerCallbackDataEXT *pCallbackData, void *) {
	if (severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eError || severity == vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning) {
		// std::cerr << "validation layer: type " << to_string(type) << " msg: " << pCallbackData->pMessage << std::endl;
        MT_LOG_ERROR("[VULKAN] validation layer: type {} msg: {}", to_string(type), pCallbackData->pMessage);
	}
	return vk::False;
}

b8 mtVulkanBackend::setupDebugMessenger() {
    if (!enableValidationLayers)
		return true;

	vk::DebugUtilsMessageSeverityFlagsEXT severityFlags(vk::DebugUtilsMessageSeverityFlagBitsEXT::eVerbose | vk::DebugUtilsMessageSeverityFlagBitsEXT::eWarning | vk::DebugUtilsMessageSeverityFlagBitsEXT::eError);
	vk::DebugUtilsMessageTypeFlagsEXT     messageTypeFlags(vk::DebugUtilsMessageTypeFlagBitsEXT::eGeneral | vk::DebugUtilsMessageTypeFlagBitsEXT::ePerformance | vk::DebugUtilsMessageTypeFlagBitsEXT::eValidation);
	vk::DebugUtilsMessengerCreateInfoEXT  debugUtilsMessengerCreateInfoEXT{};
    debugUtilsMessengerCreateInfoEXT.messageSeverity = severityFlags;
    debugUtilsMessengerCreateInfoEXT.messageType     = messageTypeFlags;
    debugUtilsMessengerCreateInfoEXT.pfnUserCallback = &debugCallback;
	_debugMessenger = _instance.createDebugUtilsMessengerEXT(debugUtilsMessengerCreateInfoEXT);
    return true;
}

b8 mtVulkanBackend::pickPhysicalDevice() {
    // Select a suitable physical device (GPU) for Vulkan operations
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    uint32_t deviceCount = 0;
    vkEnumeratePhysicalDevices(static_cast<VkInstance>(*_instance), &deviceCount, nullptr);
    if (deviceCount == 0) {
        MT_LOG_FATAL("Failed to find GPUs with Vulkan support!");
        return false;
    }

    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(static_cast<VkInstance>(*_instance), &deviceCount, devices.data());
    for (const auto& device : devices) {
        if (isDeviceSuitable(device)) {
            physicalDevice = device;
            break;
        }
    }

    if (physicalDevice == VK_NULL_HANDLE) {
        MT_LOG_FATAL("failed to find a suitable GPU!");
        return false;
    } else {
        VkPhysicalDeviceProperties deviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &deviceProperties);
        MT_LOG_INFO("Selected GPU: {}", deviceProperties.deviceName);
    }
    _physicalDevice = physicalDevice;
    return true;
}

b8 mtVulkanBackend::isDeviceSuitable(VkPhysicalDevice device) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    // b8 deviceFeature = deviceProperties.deviceType == VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU && deviceFeatures.geometryShader;
    b8 deviceFeature = true;
    if (!deviceFeature) {
        MT_LOG_ERROR("Device {} is not suitable!", deviceProperties.deviceName);
        return false;
    }
    b8 queueFamilyFeature = findQueueFamilies(device);
    return deviceFeature && queueFamilyFeature;
}

b8 mtVulkanBackend::findQueueFamilies(VkPhysicalDevice device) {
    // Find queue families that support required operations
    uint32_t queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

    std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    int i = 0;
    for (const auto& queueFamily : queueFamilies) {
        if (queueFamily.queueCount > 0 && queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            // indices.graphicsFamily = i;
            _graphicsQueueFamilyIndex = i;
        }
        VkBool32 presentSupport = false;
        vkGetPhysicalDeviceSurfaceSupportKHR(device, i, _surface, &presentSupport);
        if (queueFamily.queueCount > 0 && presentSupport) {
            _presentFamily = i;
        }
        if (_graphicsQueueFamilyIndex != -1 && _presentFamily != -1) {
            return true;
        }
    
        i++;
    }
    
    MT_LOG_ERROR("Failed to find suitable queue family!");
    return false;
}

b8 mtVulkanBackend::createLogicalDevice() {
    // Create a logical device from the selected physical device
    
    std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<s32> uniqueQueueFamilies = {_graphicsQueueFamilyIndex, _presentFamily};

    float queuePriority = 1.0f;
    for (int queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }


    // VkDeviceQueueCreateInfo queueCreateInfo = {};
    // queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
    // queueCreateInfo.queueFamilyIndex = _graphicsQueueFamilyIndex;
    // queueCreateInfo.queueCount = 1;
    // float queuePriority = 1.0f;
    // queueCreateInfo.pQueuePriorities = &queuePriority;

    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());

    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = 0;

    if (enableValidationLayers) {
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(_physicalDevice, &createInfo, nullptr, &_device) != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create logical device!");
        return false;
    }
    vkGetDeviceQueue(_device, _graphicsQueueFamilyIndex, 0, &_graphicsQueue);
    vkGetDeviceQueue(_device, _presentFamily, 0, &_presentQueue); 
    return true;
}

b8 mtVulkanBackend::createSurface() {
    // Create a Vulkan surface for rendering
    mtPlatformData data = mtApplication::getInstance()->getPlatformData();
    VkWin32SurfaceCreateInfoKHR createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    createInfo.hwnd = data.hwnd;
    createInfo.hinstance = data.hInstance;

    auto CreateWin32SurfaceKHR = (PFN_vkCreateWin32SurfaceKHR) vkGetInstanceProcAddr(static_cast<VkInstance>(*_instance), "vkCreateWin32SurfaceKHR");
    // TODO: use glfwCreateWindowSurface
    if (!CreateWin32SurfaceKHR || CreateWin32SurfaceKHR(static_cast<VkInstance>(*_instance), &createInfo, nullptr, &_surface) != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create window surface!");
        return false;
    }
    return true;
}
