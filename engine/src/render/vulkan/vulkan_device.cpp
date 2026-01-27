#define VK_USE_PLATFORM_WIN32_KHR
#include "render/vulkan/vulkan_device.h"
#include "render/vulkan/vulkan_context.h"
#include "render/vulkan/vulkan_swapchain.h"

#include "core/loggersystem.h"
#include "core/memorysystem.h"
#include "core/std_wrapper.h"

#include <set>

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

typedef struct vulkan_physical_device_queue_family_info {
    s32 graphics_family_index;
    s32 present_family_index;
    s32 compute_family_index;
    s32 transfer_family_index;
} vulkan_physical_device_queue_family_info;

static b8 isDeviceSuitable(VkPhysicalDevice device, vulkan_physical_device_queue_family_info* outQueueFamilyInfo) {
    VkPhysicalDeviceProperties deviceProperties;
    VkPhysicalDeviceFeatures deviceFeatures;
    vkGetPhysicalDeviceProperties(device, &deviceProperties);
    vkGetPhysicalDeviceFeatures(device, &deviceFeatures);

    outQueueFamilyInfo->graphics_family_index = -1;
    outQueueFamilyInfo->present_family_index = -1;
    outQueueFamilyInfo->compute_family_index = -1;
    outQueueFamilyInfo->transfer_family_index = -1;
    
    u32 queueFamilyCount = 0;
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);
    mtVector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
    vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

    MT_LOG_INFO("Found device: {}", deviceProperties.deviceName);
    MT_LOG_INFO("Graphics | Present | Compute | Transfer | Name");
    for (u32 i = 0; i < queueFamilyCount; i++) {
        s32 graphicsFamily = -1;
        s32 presentFamily = -1;
        const auto& queueFamily = queueFamilies[i];
        // TODO: Platform specific present support check
        b8 presentSupport = vkGetPhysicalDeviceWin32PresentationSupportKHR(device, i);
        
        if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
            outQueueFamilyInfo->graphics_family_index = i;
            if (presentSupport) {
                outQueueFamilyInfo->present_family_index = i;
            }
        } 
        if (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT) {
            outQueueFamilyInfo->compute_family_index = i;
        }
        if (queueFamily.queueFlags & VK_QUEUE_TRANSFER_BIT) {
            outQueueFamilyInfo->transfer_family_index = i;
        }
    }
    MT_LOG_INFO("       {:d} |       {:d} |       {:d} |        {:d} | {}", 
        outQueueFamilyInfo->graphics_family_index,
        outQueueFamilyInfo->present_family_index,
        outQueueFamilyInfo->compute_family_index,
        outQueueFamilyInfo->transfer_family_index,
        deviceProperties.deviceName
    );

    if (outQueueFamilyInfo->present_family_index != -1 && 
        outQueueFamilyInfo->present_family_index != -1 &&
        outQueueFamilyInfo->transfer_family_index != -1) {
        MT_LOG_INFO("Device meets queue requirements.");
        return true;
    }

    return false;
}

static b8 selectPhysicalDevice(mtVulkanDevice* mtDevice) {
    u32 deviceCount = 0;
    VkInstance instance = mtDevice->getContext()->getVkContext()->_instance;
    vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);
    if (deviceCount == 0) {
        MT_LOG_FATAL("Failed to find GPUs with Vulkan support!");
        return false;
    }
    mtVkDeviceContext* pDeviceCtx = mtDevice->getDeviceContext();
    std::vector<VkPhysicalDevice> devices(deviceCount);
    vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

    vulkan_physical_device_queue_family_info queueFamilyInfo = {-1, -1, -1, -1};
    for (const auto& device : devices) {
        if (isDeviceSuitable(device, &queueFamilyInfo)) {
            pDeviceCtx->_physicalDevice = device;
            break;
        }
    }
    if (pDeviceCtx->_physicalDevice == VK_NULL_HANDLE) {
        MT_LOG_FATAL("Failed to find a suitable GPU!");
        return false;
    }

    pDeviceCtx->_graphicsFamilyIndex = queueFamilyInfo.graphics_family_index;
    pDeviceCtx->_presentFamilyIndex = queueFamilyInfo.present_family_index;
    pDeviceCtx->_transferFamilyIndex = queueFamilyInfo.transfer_family_index;

    VkPhysicalDeviceProperties deviceProperties;
    vkGetPhysicalDeviceProperties(pDeviceCtx->_physicalDevice, &deviceProperties);
    MT_LOG_INFO("Selected GPU: {}", deviceProperties.deviceName);

    mtVector<VkDeviceQueueCreateInfo> queueCreateInfos;
    std::set<u32> uniqueQueueFamilies = {
        (u32)queueFamilyInfo.graphics_family_index, 
        (u32)queueFamilyInfo.present_family_index,
        (u32)queueFamilyInfo.transfer_family_index
    };
    float queuePriority = 1.0f;
    for (u32 queueFamily : uniqueQueueFamilies) {
        VkDeviceQueueCreateInfo queueCreateInfo = {};
        queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
        queueCreateInfo.queueFamilyIndex = queueFamily;
        queueCreateInfo.queueCount = 1;
        queueCreateInfo.pQueuePriorities = &queuePriority;
        queueCreateInfos.push_back(queueCreateInfo);
    }
   
    VkPhysicalDeviceFeatures deviceFeatures = {};
    VkDeviceCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
    createInfo.pQueueCreateInfos = queueCreateInfos.data();
    createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
    createInfo.pEnabledFeatures = &deviceFeatures;
    createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
    createInfo.ppEnabledExtensionNames = deviceExtensions.data();

    if (mtDevice->getContext()->getVkContext()->_enableDebug) {
        mtVector<const char*> validationLayers = {"VK_LAYER_KHRONOS_validation"};
        createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
        createInfo.ppEnabledLayerNames = validationLayers.data();
    } else {
        createInfo.enabledLayerCount = 0;
    }

    if (vkCreateDevice(pDeviceCtx->_physicalDevice, 
                       &createInfo, 
                       nullptr, 
                       &pDeviceCtx->_logicDevice) != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create logical device!");
        return false;
    }
    
    vkGetDeviceQueue(pDeviceCtx->_logicDevice, 
                     queueFamilyInfo.graphics_family_index, 
                     0, 
                     &pDeviceCtx->_graphicsQueue);
    vkGetDeviceQueue(pDeviceCtx->_logicDevice, 
                     queueFamilyInfo.present_family_index, 
                     0, 
                     &pDeviceCtx->_presentQueue); 
    vkGetDeviceQueue(pDeviceCtx->_logicDevice, 
                     queueFamilyInfo.transfer_family_index, 
                     0, 
                     &pDeviceCtx->_transferQueue);

    MT_LOG_INFO("Logical device and queues created successfully.");

    // Create command pool for graphics queue
    VkCommandPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
    poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
    
    if (vkCreateCommandPool(pDeviceCtx->_logicDevice, &poolInfo, nullptr, &pDeviceCtx->_graphicsCommandPool) != VK_SUCCESS) {
        MT_LOG_FATAL("Failed to create command pool");
        return false;
    }
    
    MT_LOG_INFO("Graphics command pool created successfully.");
    return true;
}

b8 mtVulkanDevice::initialize(mtVulkanContext* context) {
    _context = context;
    mtVkDeviceContext* pDeviceCtx = getDeviceContext();
    pDeviceCtx->_logicDevice = VK_NULL_HANDLE;
    pDeviceCtx->_physicalDevice = VK_NULL_HANDLE;

    if (!selectPhysicalDevice(this)) {
        MT_LOG_FATAL("Failed to select physical device!");
        return false;
    }

    return true;
}


b8 mtVulkanDevice::shutdown() {

    mtVkDeviceContext* pDeviceCtx = getDeviceContext();
    pDeviceCtx->_physicalDevice = VK_NULL_HANDLE;
    pDeviceCtx->_graphicsQueue = VK_NULL_HANDLE;
    pDeviceCtx->_presentQueue = VK_NULL_HANDLE;
    pDeviceCtx->_transferQueue = VK_NULL_HANDLE;

    MT_LOG_INFO("Destroying command pool...");
    vkDestroyCommandPool(pDeviceCtx->_logicDevice, pDeviceCtx->_graphicsCommandPool, nullptr);
    pDeviceCtx->_graphicsCommandPool = VK_NULL_HANDLE;

    MT_LOG_INFO("Destroying logical device...");
    if (pDeviceCtx->_logicDevice != VK_NULL_HANDLE) {
        vkDestroyDevice(pDeviceCtx->_logicDevice, nullptr);
        pDeviceCtx->_logicDevice = VK_NULL_HANDLE;
    }

    pDeviceCtx->_graphicsFamilyIndex = -1;
    pDeviceCtx->_presentFamilyIndex = -1;
    pDeviceCtx->_transferFamilyIndex = -1;

    return true;
}

void mtVulkanDevice::querySwapChainSupport(VkSurfaceKHR surface, mtVkSwapchainSupportInfo* outSupportInfo) {
    mtVkDeviceContext* pDeviceCtx = getDeviceContext();

    // Capabilities
    vkGetPhysicalDeviceSurfaceCapabilitiesKHR(pDeviceCtx->_physicalDevice, surface, &outSupportInfo->_capabilities);

    // Formats
    vkGetPhysicalDeviceSurfaceFormatsKHR(pDeviceCtx->_physicalDevice, surface, &outSupportInfo->_formatCount, nullptr);
    if (outSupportInfo->_formatCount != 0) {
        outSupportInfo->_formats = (VkSurfaceFormatKHR*)MT_ALLOCATE(mtMemTag::RENDERING, sizeof(VkSurfaceFormatKHR) * outSupportInfo->_formatCount);
        vkGetPhysicalDeviceSurfaceFormatsKHR(pDeviceCtx->_physicalDevice, surface, &outSupportInfo->_formatCount, outSupportInfo->_formats);
    } else {
        outSupportInfo->_formats = nullptr;
    }

    // Present Modes
    vkGetPhysicalDeviceSurfacePresentModesKHR(pDeviceCtx->_physicalDevice, surface, &outSupportInfo->_presentModeCount, nullptr);
    if (outSupportInfo->_presentModeCount != 0) {
        outSupportInfo->_presentModes = (VkPresentModeKHR*)MT_ALLOCATE(mtMemTag::RENDERING, sizeof(VkPresentModeKHR) * outSupportInfo->_presentModeCount);
        vkGetPhysicalDeviceSurfacePresentModesKHR(pDeviceCtx->_physicalDevice, surface, &outSupportInfo->_presentModeCount, outSupportInfo->_presentModes);
    } else {
        outSupportInfo->_presentModes = nullptr;
    }
}

s32 mtVulkanDevice::findMemoryType(u32 typeFilter, u32 propertyFlags) {
    VkPhysicalDeviceMemoryProperties memoryProperties;
    vkGetPhysicalDeviceMemoryProperties(_deviceContext._physicalDevice, &memoryProperties);

    for (u32 i = 0; i < memoryProperties.memoryTypeCount; ++i) {
        if (typeFilter & (1 << i) &&
            (memoryProperties.memoryTypes[i].propertyFlags & propertyFlags) == propertyFlags) {
            return i;
        }
    }

    MT_LOG_WARN("Unable to find suitable memory type!");
    return -1;
}

