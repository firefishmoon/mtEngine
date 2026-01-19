#include "../../defines.h"
#include "../ibackend.h"
#include <vulkan/vulkan_raii.hpp>

class MT_API mtVulkanBackend : public mtIBackend {
public:
    mtVulkanBackend() = default;
    ~mtVulkanBackend() override = default;

    b8 initialize() override;
    b8 shutdown() override;
private:
    b8 setupDebugMessenger();
    b8 pickPhysicalDevice();
    b8 isDeviceSuitable(VkPhysicalDevice device);
    b8 findQueueFamilies(VkPhysicalDevice device);
    b8 createLogicalDevice();
    b8 createSurface();
    vk::raii::Context _context;
    vk::raii::Instance _instance = nullptr;
    vk::raii::DebugUtilsMessengerEXT _debugMessenger = nullptr;
    VkDevice _device = nullptr;
    VkPhysicalDevice _physicalDevice = nullptr;
    s32 _graphicsQueueFamilyIndex = -1;
    s32 _presentFamily = -1;
    VkQueue _graphicsQueue;
    VkSurfaceKHR _surface = nullptr;
    VkQueue _presentQueue;
};
