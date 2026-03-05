#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

#include "render/vulkan/vulkan_instance.h"
#include "render/vulkan/vulkan_error.h"
#include "core/application.h"
#include "core/platform.h"

#define GLFW_INCLUDE_VULKAN
#include <glfw/glfw3.h>

class mtVulkanSurface {
public:
    mtVulkanSurface(mtVulkanInstance& instance): _instance(instance) {
        // create surface
        mtPlatformData* data = mtApplication::getInstance()->getPlatformData();
        VK_CHECK(glfwCreateWindowSurface(_instance.getHandle(), data->window, 0, &_surface));
    }
    ~mtVulkanSurface() {
        vkDestroySurfaceKHR(_instance.getHandle(), _surface, 0);
    }

    inline VkSurfaceKHR getHandle() const { return _surface; }
private:
    VkSurfaceKHR _surface;

    mtVulkanInstance& _instance;
};
