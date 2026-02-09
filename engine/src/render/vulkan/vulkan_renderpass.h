#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>


class mtVulkanRenderPass {
public:

protected:
    friend class mtVulkanFrameBuffer;
    friend class mtVulkanBackend;
    VkRenderPass _handler;
};
