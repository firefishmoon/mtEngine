#pragma once

#include "defines.h"
#include "render/render_types.h"
#include "render/vulkan/vulkan_buffer.h"
#include "render/vulkan/vulkan_pipeline.h"
#include <glm/glm.hpp>
#include <memory>
#include <vulkan/vulkan.h>

class mtVulkanContext;
class mtVulkanImage;

struct mtVulkanShaderStage {
    VkShaderModuleCreateInfo createInfo;
    VkShaderModule handle;
    VkPipelineShaderStageCreateInfo shaderStageCreateInfo;
};

#define SHADER_STAGE_COUNT 2

struct mtTextureInternalData {
    mtVulkanImage* image;
    VkSampler sampler;
};

class mtVulkanMaterialShader {
public:
    mtVulkanMaterialShader(mtVulkanContext& context);
    ~mtVulkanMaterialShader() { shutdown(); }

    void use();

    void updateGlobalState();

    void updateObject(const mtRenderGeometry& geometry);

    inline void setProjection(glm::mat4 &projection) { _globalUBO.projection = projection; }
    inline void setView(glm::mat4 &view) { _globalUBO.view = view; }

    VkRenderPass getRenderPassHandle();

private:
    b8 initialize();

    void shutdown();

    b8 createShaderModule(const char *name, const char *typeStr, VkShaderStageFlagBits shaderStageFlag, u32 stageIndex,
                          mtVulkanShaderStage *shaderStage);

    mtVulkanContext& _context;
    mtVulkanShaderStage _stages[SHADER_STAGE_COUNT];

    VkDescriptorPool _globalDescriptorPool;
    VkDescriptorSetLayout _globalDescriptorSetLayout;

    VkDescriptorPool _objectDescriptorPool;
    VkDescriptorSetLayout _objectDescriptorSetLayout;

    VkDescriptorSet _globalDescriptorSets[3];

    mtGlobalUniformObject _globalUBO;

    std::unique_ptr<mtVulkanBuffer> _globalUniformBuffer;

    // Object uniform buffers.
    std::unique_ptr<mtVulkanBuffer> object_uniform_buffer;
    // TODO: manage a free list of some kind here instead.
    u32 object_uniform_buffer_index;

    // mtVulkanPipeline _pipeline;
    std::unique_ptr<mtVulkanPipeline> _pipeline;

    // TEST CODE: temporary storage for object descriptor sets.
    VkDescriptorSet _objectDescriptorSets[3];
};
