#include "../../defines.h"
#include "../irenderbackend.h"
#include "core/std_wrapper.h"
#include "render/render_types.h"
#include "render/vulkan/vulkan_context.h"
#include "vulkan_program.h"
#include <optional>
#include <string>
#include <vector>
#include <vulkan/vulkan.h>

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    mtVector<VkSurfaceFormatKHR> formats;
    mtVector<VkPresentModeKHR> presentModes;
};

struct QueueFamilyIndices {
    std::optional<u32> graphicsFamily;
    std::optional<u32> presentFamily;

    bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

class MT_API mtVulkanBackend : public mtIRenderBackend {
  public:
    mtVulkanBackend() = default;
    ~mtVulkanBackend() override { shutdown(); };

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

    void updateGlobalState(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPosition, glm::vec4 ambientColor,
                           s32 mode) override;

    void createTexture(const u8 *pixels, mtTexture &texture) override;

    void destroyTexture(mtTexture &texture) override;

    void updateObject(const mtRenderGeometry &geometry) override;

    // Shader management
    void createShader(const u8 *data, u32 dataSize, mtShaderType type, mtShader &shader) override;
    void destroyShader(mtShader &shader) override;

    // Program management
    void createProgram(mtShader &vertexShader, mtShader &fragmentShader, mtProgramConfig &config,
                       mtProgram &program) override;
    void destroyProgram(mtProgram &program) override;

    // Uniform updates
    void updateUniform(mtProgram &program, const std::string &name, const void *data, u32 size) override;

  protected:
    b8 createImageViews();
    // b8 createRenderPass();
    //  b8 createGraphicsPipeline();
    b8 createFramebuffers();

    void uploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, mtVulkanBuffer &buffer, u64 offset, u64 size,
                         void *data);

    // b8 recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex);

    b8 checkDeviceExtensionSupport(VkPhysicalDevice device);

    // mtVector<VkFramebuffer> _swapChainFramebuffers;

    // VkRenderPass _renderPass = VK_NULL_HANDLE;

    b8 _framebufferResized = false;

    u32 MAX_FRAMES_IN_FLIGHT = 0;

    mtVulkanContext _vulkanContext;
    std::unique_ptr<mtVulkanProgramManager> _programManager;

    mtShader _shaderPool[MT_SHADER_MAX_COUNT] = {};
    mtProgram _programPool[MT_SHADER_MAX_COUNT] = {};
};
