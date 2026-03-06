#pragma once

#include "defines.h"
#include <vulkan/vulkan_core.h>

class mtVulkanContext;
class mtVulkanRenderPass;
class mtVulkanCommandBuffer;
class mtVulkanDevice;

class mtVulkanPipeline {
public:
    mtVulkanPipeline(
        mtVulkanDevice& mtVkDevice,
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

    ~mtVulkanPipeline() {
        shutdown();
    }

    void bind(mtVulkanCommandBuffer *commandBuffer, VkPipelineBindPoint bindPoint);

    inline VkPipelineLayout getPipelineLayout() { return _pipelineLayout; }

private:
    b8 initialize(
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

    // mtVulkanContext* _context;
    // handles start life null so destructor/shutdown can safely run even if
    // initialization fails or an exception is thrown.
    VkPipeline _handle = VK_NULL_HANDLE;
    VkPipelineLayout _pipelineLayout = VK_NULL_HANDLE;

    mtVulkanDevice& _mtVkDevice;
};
