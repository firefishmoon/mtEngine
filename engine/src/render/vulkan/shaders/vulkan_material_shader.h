#pragma once

#include "defines.h"
#include <vulkan/vulkan.h>
#include "render/vulkan/vulkan_buffer.h"
#include "render/vulkan/vulkan_pipeline.h"
#include "render/render_types.h"
#include <glm/glm.hpp>

class mtVulkanContext;

struct mtVulkanShaderStage {
    VkShaderModuleCreateInfo createInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
};

#define SHADER_STAGE_COUNT 2

class mtVulkanMaterialShader {
public:
    b8 initialize(mtVulkanContext* context);

    void shutdown();

    void use();

    void updateGlobalState();

    inline void setProjection(glm::mat4& projection) { _globalUBO.projection = projection; }
    inline void setView(glm::mat4& view) { _globalUBO.view = view; }
private:
    b8 createShaderModule(
        const char* name,
        const char* typeStr,
        VkShaderStageFlagBits shaderStageFlag,
        u32 stageIndex,
        mtVulkanShaderStage* shaderStage
    );

    mtVulkanContext* _context;
    mtVulkanShaderStage _stages[SHADER_STAGE_COUNT];

    VkDescriptorPool _globalDescriptorPool;
    VkDescriptorSetLayout _globalDescriptorSetLayout;

    VkDescriptorSet _globalDescriptorSets[3];

    GlobalUniformObject _globalUBO;

    mtVulkanBuffer _globalUniformBuffer;

    mtVulkanPipeline _pipeline;
};
