#pragma once

#include "defines.h"
#include <vulkan/vulkan_core.h>

class mtVulkanContext;
class mtVulkanRenderPass;
class mtVulkanCommandBuffer;

class mtVulkanPipeline {
public:
    mtVulkanPipeline():_context(0), _handle(0), _pipelineLayout(0) {}
    b8 initialize(
        mtVulkanContext *context,
        mtVulkanRenderPass *renderpass,
        u32 attributeCount,
        VkVertexInputAttributeDescription* attributes,
        u32 descriptorSetLayoutCount,
        VkDescriptorSetLayout* descriptorSetLayouts,
        u32 stageCount,
        VkPipelineShaderStageCreateInfo *stages,
        VkViewport viewport,
        VkRect2D scissor,
        b8 isWireFrame
    );

    void shutdown();

    void bind(mtVulkanCommandBuffer *commandBuffer, VkPipelineBindPoint bindPoint);

    inline VkPipelineLayout getPipelineLayout() { return _pipelineLayout; }

private:
    mtVulkanContext* _context;
    VkPipeline _handle;
    VkPipelineLayout _pipelineLayout;
};
