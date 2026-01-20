#include "../../defines.h"
#include "../ibackend.h"
#include <vector>
#include <optional>
#include <string>
#include <vulkan/vulkan.h>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;
    
    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

class MT_API mtVulkanBackend : public mtIBackend {
public:
    mtVulkanBackend() = default;
    ~mtVulkanBackend() override = default;

    b8 initialize() override;
    b8 shutdown() override;
    
    b8 renderFrame() override;
    b8 recreateSwapChain();
    b8 waitDeviceIdle();
    
    VkDevice getDevice() const { return _device; }
    VkPhysicalDevice getPhysicalDevice() const { return _physicalDevice; }
    VkQueue getGraphicsQueue() const { return _graphicsQueue; }
    VkQueue getPresentQueue() const { return _presentQueue; }
    VkSurfaceKHR getSurface() const { return _surface; }
    VkInstance getInstance() const { return _instance; }
    
private:
    b8 setupDebugMessenger();
    b8 pickPhysicalDevice();
    b8 isDeviceSuitable(VkPhysicalDevice device);
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    b8 createLogicalDevice();
    b8 createSurface();
    b8 createSwapChain();
    b8 createImageViews();
    b8 createRenderPass();
    b8 createGraphicsPipeline();
    b8 createFramebuffers();
    b8 createCommandPool();
    b8 createCommandBuffers();
    b8 createSyncObjects();
    
    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device);
    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats);
    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes);
    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities);
    
    b8 recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    
    b8 checkDeviceExtensionSupport(VkPhysicalDevice device);
    std::vector<char> readFile(const std::string& filename);
    VkShaderModule createShaderModule(const std::vector<char>& code);

    VkInstance _instance = VK_NULL_HANDLE;
    VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    VkDevice _device = VK_NULL_HANDLE;
    VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    VkQueue _graphicsQueue = VK_NULL_HANDLE;
    VkSurfaceKHR _surface = VK_NULL_HANDLE;
    VkQueue _presentQueue = VK_NULL_HANDLE;

    VkCommandPool _commandPool = VK_NULL_HANDLE;
    std::vector<VkCommandBuffer> _commandBuffers;

    VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    std::vector<VkImage> _swapChainImages;
    VkFormat _swapChainImageFormat;
    VkExtent2D _swapChainExtent;
    std::vector<VkImageView> _swapChainImageViews;
    std::vector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

    std::vector<VkSemaphore> _imageAvailableSemaphores;
    std::vector<VkSemaphore> _renderFinishedSemaphores;
    std::vector<VkFence> _inFlightFences;
    
    u32 _currentFrame = 0;
    b8 _framebufferResized = false;
    
    static const u32 MAX_FRAMES_IN_FLIGHT = 2;
};
