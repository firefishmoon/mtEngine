#include <stdexcept>
#include <algorithm>
#include <GLFW/glfw3.h>
#include "vulkan_backend.h"
#include "core/loggersystem.h"

b8 mtVulkanBackend::initialize() {
    // Initialize Vulkan-specific resources and state
    MT_LOG_INFO("Initializing Vulkan Backend");
    VkInstance instance;
    VkApplicationInfo appInfo{};
    appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
    appInfo.pApplicationName = "Hello Triangle";
    appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.pEngineName = "No Engine";
    appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
    appInfo.apiVersion = VK_API_VERSION_1_0;

    // Get the required instance extensions from GLFW.
	uint32_t glfwExtensionCount = 0;
	auto     glfwExtensions     = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

	// Check if the required GLFW extensions are supported by the Vulkan implementation.
	auto extensionProperties = _context.enumerateInstanceExtensionProperties();
	for (uint32_t i = 0; i < glfwExtensionCount; ++i) {
		if (std::ranges::none_of(extensionProperties,
			                    [glfwExtension = glfwExtensions[i]](auto const &extensionProperty) { return strcmp(extensionProperty.extensionName, glfwExtension) == 0; })) {
				throw std::runtime_error("Required GLFW extension not supported: " + std::string(glfwExtensions[i]));
		}
	}

    VkInstanceCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
    createInfo.pApplicationInfo = &appInfo;
    createInfo.enabledExtensionCount = glfwExtensionCount;
    createInfo.ppEnabledExtensionNames = glfwExtensions;

    createInfo.enabledLayerCount = 0;

    _instance = vk::raii::Instance(_context, createInfo);
    MT_LOG_INFO("Vulkan Backend Initialized");

    return true;
}
b8 mtVulkanBackend::shutdown() {
    // Clean up Vulkan-specific resources and state
    return true;
}

