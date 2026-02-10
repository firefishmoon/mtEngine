#pragma once

#include "defines.h"
#include "core/std_wrapper.h"
#include <vulkan/vulkan.h>

class mtVulkanRenderPass;
class mtVulkanContext;

class mtVulkanFrameBuffer {
public:
    b8 initialize(mtVulkanContext* context,
                  mtVulkanRenderPass *renderpass,
                  u32 width,
                  u32 height,
                  mtVector<VkImageView>& attachments);

    void shutdown();
protected:
    friend class mtVulkanBackend;
    friend class mtVulkanRenderPass;

    mtVulkanContext *_context;
    VkFramebuffer _handle;
    u32 _attachmentCount;
    mtVector<VkImageView> _attachments;
    mtVulkanRenderPass* _renderpass;
};
