#include "../../defines.h"
#include "../irenderbackend.h"
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

class MT_API mtVulkanBackend : public mtIRenderBackend {
public:
    mtVulkanBackend() = default;
    ~mtVulkanBackend() override {
        shutdown();
    };

    b8 initialize(u32 width, u32 height) override;
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

    void updateGlobalState(
        glm::mat4 projection,
        glm::mat4 view,
        glm::vec3 viewPosition,
        glm::vec4 ambientColor,
        s32 mode
    ) override;

    mtTextureHandle createTexture(const std::string& name, s32 width, s32 height, s32 channelCount, const u8* pixels, b8 hasTransparency) override;

    void destroyTexture(mtTextureHandle& texture) override;

    void updateObject(mtGeometryData& geometry) override;

protected:
    b8 createImageViews();
    //b8 createRenderPass();
    // b8 createGraphicsPipeline();
    b8 createFramebuffers();

    void uploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, mtVulkanBuffer& buffer, u64 offset, u64 size, void* data);

    // b8 recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);

    b8 checkDeviceExtensionSupport(VkPhysicalDevice device);

    // mtVector<VkFramebuffer> _swapChainFramebuffers;

    // VkRenderPass _renderPass = VK_NULL_HANDLE;

    b8 _framebufferResized = false;

    u32 MAX_FRAMES_IN_FLIGHT = 0;

    mtVulkanContext _vulkanContext;
};
