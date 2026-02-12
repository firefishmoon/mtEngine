#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>

class mtVulkanContext;
class mtVulkanFrameBuffer;
class mtVulkanCommandBuffer;

class mtVulkanRenderPass {
public:
    inline VkRenderPass getHandle() const { return _handle; }
    void setExtent(u32 w, u32 h) { _w = (f32)w; _h = (f32)h; }
    b8 initialize(mtVulkanContext* context,
                  f32 x, f32 y, f32 w, f32 h,
                  f32 r, f32 g, f32 b, f32 a,
                  f32 depth, f32 stencil);

    b8 shutdown();

    void begin(mtVulkanCommandBuffer& commandBuffer, mtVulkanFrameBuffer& frameBuffer);
    void end(mtVulkanCommandBuffer& commandBuffer);

protected:
    VkRenderPass _handle;

    mtVulkanContext* _context;

    f32 _x, _y, _w, _h, _r, _g, _b, _a, _depth, _stencil;
};
