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
    ~mtVulkanBackend() override {
        shutdown();
    };

    b8 initialize() override;
    b8 createSurface();
    b8 shutdown() override;
    
    // b8 renderFrame() override;
    b8 onResize(u32 width, u32 height) override {
        _framebufferResized = true;
        _vulkanContext.setWidth(static_cast<u16>(width));
        _vulkanContext.setHeight(static_cast<u16>(height));
        return true;
    }
    b8 renderPrepare() override;
    b8 renderBegin() override;
    b8 renderEnd() override;
    b8 renderPresent() override;
    b8 recreateSwapChain();
    
protected:
    b8 createImageViews();
    //b8 createRenderPass();
    b8 createGraphicsPipeline();
    b8 createFramebuffers();
    
    
    b8 recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);
    
    b8 checkDeviceExtensionSupport(VkPhysicalDevice device);

    // mtVector<VkFramebuffer> _swapChainFramebuffers;

    VkRenderPass _renderPass = VK_NULL_HANDLE;

    b8 _framebufferResized = false;
    
    u32 MAX_FRAMES_IN_FLIGHT = 0;

    mtVulkanContext _vulkanContext;
};
