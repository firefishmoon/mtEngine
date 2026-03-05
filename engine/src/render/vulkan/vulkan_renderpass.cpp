#include "vulkan_renderpass.h"
#include "vulkan_framebuffer.h"
#include "vulkan_command_buffer.h"
#include "vulkan_device.h"
#include "vulkan_swapchain.h"
#include "vulkan_context.h"

#include "core/loggersystem.h"

mtVulkanRenderPass::mtVulkanRenderPass(
        mtVulkanDevice& mtVkDevice,
        mtVulkanSwapChain& mtVkSwapChain,
        f32 x, f32 y, f32 w, f32 h,
        f32 r, f32 g, f32 b, f32 a,
        f32 depth, f32 stencil
    ) : _mtVkDevice(mtVkDevice), _mtVkSwapChain(mtVkSwapChain) {
    if (!initialize(x, y, w, h, r, g, b, a, depth, stencil)) {
        throw std::runtime_error("Failed to create Vulkan Render Pass!");
    }
}

b8 mtVulkanRenderPass::initialize(f32 x, f32 y, f32 w, f32 h,
                                f32 r, f32 g, f32 b, f32 a,
                                f32 depth, f32 stencil) {

    _x = x;
    _y = y;
    _w = w;
    _h = h;
    _r = r;
    _g = g;
    _b = b;
    _a = a;
    _depth = depth;
    _stencil = stencil;
    const u32 attachment_description_count = 2;
    VkAttachmentDescription attachment_descriptions[attachment_description_count];

    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _mtVkSwapChain.getImageFormat().format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

    attachment_descriptions[0] = colorAttachment;

    VkAttachmentReference colorAttachmentRef{};
    colorAttachmentRef.attachment = 0;
    colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

    VkSubpassDescription subpass{};
    subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
    subpass.colorAttachmentCount = 1;
    subpass.pColorAttachments = &colorAttachmentRef;

    VkSubpassDependency dependency{};
    dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
    dependency.dstSubpass = 0;
    dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.srcAccessMask = 0;
    dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_COLOR_ATTACHMENT_READ_BIT;


    VkAttachmentDescription depth_attachment = {};
    depth_attachment.format = _mtVkDevice.getDepthFormat();
    depth_attachment.samples = VK_SAMPLE_COUNT_1_BIT;
    depth_attachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    depth_attachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    depth_attachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    depth_attachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    depth_attachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    attachment_descriptions[1] = depth_attachment;

    VkAttachmentReference depth_attachment_reference;
    depth_attachment_reference.attachment = 1;
    depth_attachment_reference.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

    subpass.pDepthStencilAttachment = &depth_attachment_reference;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 2;
    renderPassInfo.pAttachments = attachment_descriptions;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkDevice device = _mtVkDevice.getLogicalDevice();
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &_handle) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create render pass");
        return false;
    }

    return true;
}

b8 mtVulkanRenderPass::shutdown() {
    vkDestroyRenderPass(_mtVkDevice.getLogicalDevice(), _handle, 0);
    return true;
}

void mtVulkanRenderPass::begin(mtVulkanCommandBuffer& commandBuffer, mtVulkanFrameBuffer& frameBuffer) {
    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _handle;
    renderPassInfo.framebuffer = frameBuffer.getHandle(); //_swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {(s32)_x, (s32)_y};
    renderPassInfo.renderArea.extent = {(u32)_w, (u32)_h};

    VkClearValue clearColor[2]; //= {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    clearColor[0].color.float32[0] = _r;
    clearColor[0].color.float32[1] = _g;
    clearColor[0].color.float32[2] = _b;
    clearColor[0].color.float32[3] = _a;
    clearColor[1].depthStencil.depth = _depth;
    clearColor[1].depthStencil.stencil = _stencil;
    renderPassInfo.clearValueCount = 2;
    renderPassInfo.pClearValues = clearColor;

    vkCmdBeginRenderPass(commandBuffer.getHandle(), &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
}

void mtVulkanRenderPass::end(mtVulkanCommandBuffer& commandBuffer) {
    vkCmdEndRenderPass(commandBuffer.getHandle());
}
