#include "vulkan_framebuffer.h"
#include "vulkan_renderpass.h"
#include "vulkan_error.h"
#include "vulkan_context.h"
#include <vulkan/vulkan_core.h>

b8 mtVulkanFrameBuffer::initialize(mtVulkanContext* context,
                  mtVulkanRenderPass *renderpass,
                  u32 width,
                  u32 height,
                  mtVector<VkImageView>& attachments) {
    _context = context;
    for (VkImageView iv : attachments) {
        _attachments.emplace_back(iv);
    }
    _renderpass = renderpass;

    VkFramebufferCreateInfo createInfo = {VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO};
    createInfo.renderPass = renderpass->_handle;
    createInfo.attachmentCount = _attachments.size();
    createInfo.pAttachments = _attachments.data();
    createInfo.width = width;
    createInfo.height = height;
    createInfo.layers = 1;

    VK_CHECK(vkCreateFramebuffer(_context->_vulkanDevice._logicDevice,
                                 &createInfo, 
                                 0, 
                                 &_handle));
    
    return true;
}

void mtVulkanFrameBuffer::shutdown() {
    vkDestroyFramebuffer(_context->_vulkanDevice._logicDevice, _handle, 0);
    _handle = 0;
    _attachments.clear();
    _renderpass = 0;
}
