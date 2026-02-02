#include "../../defines.h"
#include "../ibackend.h"
#include <vector>
#include <optional>
#include <string>
#include <vulkan/vulkan.h>
#include "render/vulkan/vulkan_context.h"
#include "core/std_wrapper.h"

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    mtVector<VkSurfaceFormatKHR> formats;
    mtVector<VkPresentModeKHR> presentModes;
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
    b8 createSurface();
    b8 shutdown() override;
    
    b8 renderFrame() override;
    b8 onResize(u32 width, u32 height) override {
        _framebufferResized = true;
        _vulkanContext._width = width;
        _vulkanContext._height = height;
        return true;
    }
    b8 renderPrepare() override;
    b8 renderBegin() override;
    b8 renderEnd() override;
    b8 renderPresent() override;
    b8 recreateSwapChain();
    // b8 waitDeviceIdle();
    
    // VkDevice getDevice() const { return _device; }
    // VkPhysicalDevice getPhysicalDevice() const { return _physicalDevice; }
    // VkQueue getGraphicsQueue() const { return _graphicsQueue; }
    // VkQueue getPresentQueue() const { return _presentQueue; }
    // VkSurfaceKHR getSurface() const { return _surface; }
    // VkInstance getInstance() const { return _instance; }
    
private:
    // b8 pickPhysicalDevice();
    // b8 isDeviceSuitable(VkPhysicalDevice device);
    // QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device);
    // b8 createLogicalDevice();
    // b8 createSwapChain();
    b8 createImageViews();
    b8 createRenderPass();
    b8 createGraphicsPipeline();
    b8 createFramebuffers();
    // b8 createCommandPool();
    b8 createCommandBuffers();
    b8 createSyncObjects();
    
    
    b8 recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    
    b8 checkDeviceExtensionSupport(VkPhysicalDevice device);
    // mtVector<char> readFile(const std::string& filename);
    // VkShaderModule createShaderModule(const mtVector<char>& code);

    // VkInstance _instance = VK_NULL_HANDLE;
    // VkDebugUtilsMessengerEXT _debugMessenger = VK_NULL_HANDLE;
    // VkDevice _device = VK_NULL_HANDLE;
    // VkPhysicalDevice _physicalDevice = VK_NULL_HANDLE;
    // VkQueue _graphicsQueue = VK_NULL_HANDLE;
    // VkSurfaceKHR _surface = VK_NULL_HANDLE;
    // VkQueue _presentQueue = VK_NULL_HANDLE;

    // VkCommandPool _commandPool = VK_NULL_HANDLE;
    // mtVector<VkCommandBuffer> _commandBuffers;

    // VkSwapchainKHR _swapChain = VK_NULL_HANDLE;
    // mtVector<VkImage>* _pSwapChainImages;
    // VkFormat _swapChainImageFormat;
    // VkExtent2D _swapChainExtent;
    // mtVector<VkImageView>* _pSwapChainImageViews;
    mtVector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass = VK_NULL_HANDLE;
    // VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;
    // VkPipeline _graphicsPipeline = VK_NULL_HANDLE;

    mtVector<VkSemaphore> _imageAvailableSemaphores;
    mtVector<VkSemaphore> _renderFinishedSemaphores;
    mtVector<VkFence> _inFlightFences;
    
    u32 _currentFrame = 0;
    b8 _framebufferResized = false;
    
    u32 MAX_FRAMES_IN_FLIGHT = 0;

    mtVulkanContext _vulkanContext;
};
