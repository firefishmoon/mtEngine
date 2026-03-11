#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;
class mtVulkanFrameBuffer;
class mtVulkanCommandBuffer;
class mtVulkanDevice;
class mtVulkanSwapChain;

class mtVulkanRenderPass {
public:
    mtVulkanRenderPass(
        mtVulkanDevice& mtVkDevice,
        mtVulkanSwapChain& mtVkSwapChain,
        f32 x, f32 y, f32 w, f32 h,
        f32 r, f32 g, f32 b, f32 a,
        f32 depth, f32 stencil
    );

    ~mtVulkanRenderPass() {
        shutdown();
    }

    inline VkRenderPass getHandle() const { return _handle; }
    void setExtent(u32 w, u32 h) { _w = (f32)w; _h = (f32)h; }

    void begin(mtVulkanCommandBuffer& commandBuffer, mtVulkanFrameBuffer& frameBuffer);
    void end(mtVulkanCommandBuffer& commandBuffer);

protected:
    b8 initialize(f32 x, f32 y, f32 w, f32 h,
                  f32 r, f32 g, f32 b, f32 a,
                  f32 depth, f32 stencil);
    b8 shutdown();

    VkRenderPass _handle;


    f32 _x, _y, _w, _h, _r, _g, _b, _a, _depth, _stencil;

    mtVulkanDevice& _mtVkDevice;
    mtVulkanSwapChain& _mtVkSwapChain;
};
