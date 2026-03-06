#include "vulkan_material_shader.h"
#include "core/loggersystem.h"
#include "render/vulkan/vulkan_context.h"
#include "render/vulkan/vulkan_error.h"
#include <string>
#include <fstream>
#include <glm/glm.hpp>

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.Shader"

mtVulkanMaterialShader::mtVulkanMaterialShader(mtVulkanContext& context) : _context(context) {
    if (!initialize()) {
        MT_LOG_ERROR("Failed to initialize Vulkan material shader.");
        throw std::runtime_error("Failed to initialize Vulkan material shader.");
    }
}

b8 mtVulkanMaterialShader::initialize() {
    // Shader module init per stage.
    char stage_type_strs[SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    for (u32 i = 0; i < SHADER_STAGE_COUNT; ++i) {
        if (!createShaderModule(BUILTIN_SHADER_NAME_OBJECT, stage_type_strs[i], stage_types[i], i, _stages)) {
            MT_LOG_ERROR("Unable to create {} shader module for '{}'.", stage_type_strs[i], BUILTIN_SHADER_NAME_OBJECT);
            return false;
        }
    }

    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();
    // Global Descriptors
    VkDescriptorSetLayoutBinding global_ubo_layout_binding;
    global_ubo_layout_binding.binding = 0;
    global_ubo_layout_binding.descriptorCount = 1;
    global_ubo_layout_binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_ubo_layout_binding.pImmutableSamplers = 0;
    global_ubo_layout_binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    VkDescriptorSetLayoutCreateInfo global_layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    global_layout_info.bindingCount = 1;
    global_layout_info.pBindings = &global_ubo_layout_binding;
    VK_CHECK(vkCreateDescriptorSetLayout(
        device,
        &global_layout_info,
        0,
        &_globalDescriptorSetLayout));

    // Global descriptor pool: Used for global items such as view/projection matrix.
    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = _context.getVulkanSwapChain()->getImageCount();

    VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes = &global_pool_size;
    global_pool_info.maxSets = _context.getVulkanSwapChain()->getImageCount();
    VK_CHECK(vkCreateDescriptorPool(
        device,
        &global_pool_info,
        0,
        &_globalDescriptorPool));

     // Pipeline creation
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)_context.getHeight();
    viewport.width = (f32)_context.getWidth();
    viewport.height = -(f32)_context.getHeight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = _context.getWidth();
    scissor.extent.height = _context.getHeight();

    // Attributes
    u32 offset = 0;
    const s32 attribute_count = 1;
    VkVertexInputAttributeDescription attribute_descriptions[attribute_count];
    // Position
    VkFormat formats[attribute_count] = {
        VK_FORMAT_R32G32B32_SFLOAT};
    u64 sizes[attribute_count] = {
        sizeof(glm::vec3)
    };
    for (u32 i = 0; i < attribute_count; ++i) {
        attribute_descriptions[i].binding = 0;   // binding index - should match binding desc
        attribute_descriptions[i].location = i;  // attrib location
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Desciptor set layouts.
    const s32 descriptor_set_layout_count = 1;
    VkDescriptorSetLayout layouts[1] = {
        _globalDescriptorSetLayout};

    // Stages
    // NOTE: Should match the number of shader->stages.
    VkPipelineShaderStageCreateInfo stage_create_infos[SHADER_STAGE_COUNT];
    // kzero_memory(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType = _stages[i].shaderStageCreateInfo.sType;
        stage_create_infos[i] = _stages[i].shaderStageCreateInfo;
    }

    _pipeline = std::make_unique<mtVulkanPipeline>(
        *_context.getVulkanDevice(),
        _context.getMainRenderPass(),
        attribute_count,
        attribute_descriptions,
        descriptor_set_layout_count,
        layouts,
        SHADER_STAGE_COUNT,
        stage_create_infos,
        viewport,
        scissor,
        false
    );

    // if (!_pipeline.initialize(
    //     context,
    //     context->getMainRenderPass(),
    //     attribute_count,
    //     attribute_descriptions,
    //     descriptor_set_layout_count,
    //     layouts,
    //     SHADER_STAGE_COUNT,
    //     stage_create_infos,
    //     viewport,
    //     scissor,
    //     false
    // )) {
    //     MT_LOG_ERROR("Failed to load graphics pipeline for shader.");
    //     return false;
    // }

    // Create uniform buffer.
    // if (!_globalUniformBuffer.initialize(
    //     context,
    //     sizeof(GlobalUniformObject),
    //     VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
    //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
    //     true
    // )) {
    //     MT_LOG_ERROR("Vulkan buffer creation failed for shader.");
    //     return false;
    // }

    _globalUniformBuffer = std::make_unique<mtVulkanBuffer>(
        *_context.getVulkanDevice(),
        sizeof(mtGlobalUniformObject),
        VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true
    );

    // Allocate global descriptor sets.
    VkDescriptorSetLayout global_layouts[3] = {
        _globalDescriptorSetLayout,
        _globalDescriptorSetLayout,
        _globalDescriptorSetLayout,
    };

    VkDescriptorSetAllocateInfo alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    alloc_info.descriptorPool = _globalDescriptorPool;
    alloc_info.descriptorSetCount = 3;
    alloc_info.pSetLayouts = global_layouts;
    VK_CHECK(vkAllocateDescriptorSets(device, &alloc_info, _globalDescriptorSets));


    return true;
}

b8 mtVulkanMaterialShader::createShaderModule(
        const char* name,
        const char* typeStr,
        VkShaderStageFlagBits shaderStageFlag,
        u32 stageIndex,
        mtVulkanShaderStage* shaderStages
    ) {

    std::string fileName = std::format("assets/shaders/{}.{}.spv", name, typeStr);
    shaderStages[stageIndex].createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;

    std::ifstream file(fileName, std::ios::ate | std::ios::binary);
    if (!file.is_open()) {
        MT_LOG_ERROR("Failed to open file: {}", fileName.c_str());
        return {};
    }
    size_t fileSize = (size_t)file.tellg();
    std::vector<char> buffer(fileSize);

    file.seekg(0);
    file.read(buffer.data(), fileSize);
    file.close();
    shaderStages[stageIndex].createInfo.codeSize = fileSize;
    shaderStages[stageIndex].createInfo.pCode = (u32*)buffer.data();

    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();

    VK_CHECK(vkCreateShaderModule(
        device,
        &shaderStages[stageIndex].createInfo,
        0,
        &shaderStages[stageIndex].handle));

    // Shader stage info
    shaderStages[stageIndex].shaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
    shaderStages[stageIndex].shaderStageCreateInfo.stage = shaderStageFlag;
    shaderStages[stageIndex].shaderStageCreateInfo.module = shaderStages[stageIndex].handle;
    shaderStages[stageIndex].shaderStageCreateInfo.pName = "main";

    return true;
}

void mtVulkanMaterialShader::shutdown() {

    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();
    // _globalUniformBuffer.shutdown();
    // _pipeline.shutdown();
    // Destroy global descriptor pool.
    vkDestroyDescriptorPool(device, _globalDescriptorPool, 0);

    // Destroy descriptor set layouts.
    vkDestroyDescriptorSetLayout(device, _globalDescriptorSetLayout, 0);

    // Destroy shader modules.
    for (u32 i = 0; i < SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(device, _stages[i].handle, 0);
        _stages[i].handle = 0;
    }
}

void mtVulkanMaterialShader::use() {
    auto _pCommandBuffers = _context.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];
    _pipeline->bind(&commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);

}

void mtVulkanMaterialShader::updateGlobalState() {
    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();
    u32 image_index = _context.getCurrentFrame();

    // Configure the descriptors for the given index.
    u32 range = sizeof(mtGlobalUniformObject);
    u64 offset = 0;

    // Copy data to buffer
    _globalUniformBuffer->loadData(offset, range, 0, &_globalUBO);

    VkDescriptorBufferInfo bufferInfo;
    bufferInfo.buffer = _globalUniformBuffer->getHandle();
    bufferInfo.offset = offset;
    bufferInfo.range = range;

    // Update descriptor sets.
    VkWriteDescriptorSet descriptor_write = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor_write.dstSet = _globalDescriptorSets[image_index];
    descriptor_write.dstBinding = 0;
    descriptor_write.dstArrayElement = 0;
    descriptor_write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor_write.descriptorCount = 1;
    descriptor_write.pBufferInfo = &bufferInfo;

    vkUpdateDescriptorSets(device, 1, &descriptor_write, 0, 0);

    // Bind the global descriptor set to the command buffer.
    auto _pCommandBuffers = _context.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];
    VkCommandBuffer command_buffer = commandBuffer.getHandle();
    VkDescriptorSet global_descriptor = _globalDescriptorSets[image_index];
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1, &global_descriptor, 0, 0);
}


void mtVulkanMaterialShader::updateObject(glm::mat4 model) {
    u32 image_index = _context.getCurrentFrame();
    auto _pCommandBuffers = _context.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];

    vkCmdPushConstants(commandBuffer.getHandle(), _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0, sizeof(glm::mat4), &model);
}
