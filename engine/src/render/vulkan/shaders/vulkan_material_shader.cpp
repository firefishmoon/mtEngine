#include "vulkan_material_shader.h"
#include "core/loggersystem.h"
#include "render/vulkan/vulkan_context.h"
#include "render/vulkan/vulkan_error.h"
#include <fstream>
#include <glm/glm.hpp>
#include <string>

#define BUILTIN_SHADER_NAME_OBJECT "Builtin.Shader"

mtVulkanMaterialShader::mtVulkanMaterialShader(mtVulkanContext &context) : _context(context) {
    if (!initialize()) {
        MT_LOG_ERROR("Failed to initialize Vulkan material shader.");
        // throw std::runtime_error("Failed to initialize Vulkan material shader.");
    }
}

b8 mtVulkanMaterialShader::initialize() {
    // Shader module init per stage.
    char stage_type_strs[SHADER_STAGE_COUNT][5] = {"vert", "frag"};
    VkShaderStageFlagBits stage_types[SHADER_STAGE_COUNT] = {VK_SHADER_STAGE_VERTEX_BIT, VK_SHADER_STAGE_FRAGMENT_BIT};

    memset(_stages, 0, sizeof(mtVulkanShaderStage) * SHADER_STAGE_COUNT);

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
    VK_CHECK(vkCreateDescriptorSetLayout(device, &global_layout_info, 0, &_globalDescriptorSetLayout));

    // Global descriptor pool: Used for global items such as view/projection matrix.
    VkDescriptorPoolSize global_pool_size;
    global_pool_size.type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    global_pool_size.descriptorCount = _context.getVulkanSwapChain()->getImageCount();

    VkDescriptorPoolCreateInfo global_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    global_pool_info.poolSizeCount = 1;
    global_pool_info.pPoolSizes = &global_pool_size;
    global_pool_info.maxSets = _context.getVulkanSwapChain()->getImageCount();
    VK_CHECK(vkCreateDescriptorPool(device, &global_pool_info, 0, &_globalDescriptorPool));

    // Local/Object Descriptors
    const u32 VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT = 2; // TODO: this should be dynamic based on the shader, but for
                                                         // now we can hardcode it since we only have one shader.
    const u32 VULKAN_OBJECT_MAX_OBJECT_COUNT = 100; // TODO: this should be dynamic based on the number of objects we
                                                    // want to render, but for now we can hardcode it.
    const u32 local_sampler_count = 1;
    VkDescriptorType descriptor_types[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT] = {
        VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // Binding 0 - uniform buffer
        VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // Binding 1 - Diffuse sampler layout.
    };
    VkDescriptorSetLayoutBinding bindings[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
    // kzero_memory(&bindings, sizeof(VkDescriptorSetLayoutBinding) * VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT);
    for (u32 i = 0; i < VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT; ++i) {
        bindings[i].binding = i;
        bindings[i].descriptorCount = 1;
        bindings[i].descriptorType = descriptor_types[i];
        bindings[i].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }

    VkDescriptorSetLayoutCreateInfo layout_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO};
    layout_info.bindingCount = VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT;
    layout_info.pBindings = bindings;
    VK_CHECK(vkCreateDescriptorSetLayout(device, &layout_info, 0, &_objectDescriptorSetLayout));

    // Local/Object descriptor pool: Used for object-specific items like diffuse colour
    VkDescriptorPoolSize object_pool_sizes[2];
    // The first section will be used for uniform buffers
    object_pool_sizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    object_pool_sizes[0].descriptorCount = VULKAN_OBJECT_MAX_OBJECT_COUNT;
    // The second section will be used for image samplers.
    object_pool_sizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    object_pool_sizes[1].descriptorCount = local_sampler_count * VULKAN_OBJECT_MAX_OBJECT_COUNT;

    VkDescriptorPoolCreateInfo object_pool_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO};
    object_pool_info.poolSizeCount = 2;
    object_pool_info.pPoolSizes = object_pool_sizes;
    object_pool_info.maxSets = VULKAN_OBJECT_MAX_OBJECT_COUNT;

    // Create object descriptor pool.
    VK_CHECK(vkCreateDescriptorPool(device, &object_pool_info, 0, &_objectDescriptorPool));

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
    const s32 attribute_count = 2;
    VkVertexInputAttributeDescription attribute_descriptions[attribute_count];
    // Position
    VkFormat formats[attribute_count] = {VK_FORMAT_R32G32B32_SFLOAT, VK_FORMAT_R32G32_SFLOAT};
    u64 sizes[attribute_count] = {sizeof(glm::vec3), sizeof(glm::vec2)};
    for (u32 i = 0; i < attribute_count; ++i) {
        attribute_descriptions[i].binding = 0;  // binding index - should match binding desc
        attribute_descriptions[i].location = i; // attrib location
        attribute_descriptions[i].format = formats[i];
        attribute_descriptions[i].offset = offset;
        offset += sizes[i];
    }

    // Desciptor set layouts.
    const s32 descriptor_set_layout_count = 2;
    VkDescriptorSetLayout layouts[2] = {_globalDescriptorSetLayout, _objectDescriptorSetLayout};

    // Stages
    // NOTE: Should match the number of shader->stages.
    VkPipelineShaderStageCreateInfo stage_create_infos[SHADER_STAGE_COUNT];
    // kzero_memory(stage_create_infos, sizeof(stage_create_infos));
    for (u32 i = 0; i < SHADER_STAGE_COUNT; ++i) {
        stage_create_infos[i].sType = _stages[i].shaderStageCreateInfo.sType;
        stage_create_infos[i] = _stages[i].shaderStageCreateInfo;
    }

    _pipeline = std::make_unique<mtVulkanPipeline>(
        *_context.getVulkanDevice(), _context.getMainRenderPass(), attribute_count, attribute_descriptions,
        descriptor_set_layout_count, layouts, SHADER_STAGE_COUNT, stage_create_infos, viewport, scissor, false);

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
    //     VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
    //     VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, true
    // )) {
    //     MT_LOG_ERROR("Vulkan buffer creation failed for shader.");
    //     return false;
    // }

    _globalUniformBuffer =
        std::make_unique<mtVulkanBuffer>(*_context.getVulkanDevice(), sizeof(mtGlobalUniformObject),
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         true);

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

    object_uniform_buffer =
        std::make_unique<mtVulkanBuffer>(*_context.getVulkanDevice(), sizeof(mtObjectUniformObject),
                                         VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
                                         VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
                                             VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
                                         true);

    VkDescriptorSetLayout object_layouts[3] = {
        _objectDescriptorSetLayout,
        _objectDescriptorSetLayout,
        _objectDescriptorSetLayout,
    };

    VkDescriptorSetAllocateInfo obj_alloc_info = {VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO};
    obj_alloc_info.descriptorPool = _objectDescriptorPool;
    obj_alloc_info.descriptorSetCount = 3;
    obj_alloc_info.pSetLayouts = object_layouts;
    VK_CHECK(vkAllocateDescriptorSets(device, &obj_alloc_info, _objectDescriptorSets));
    return true;
}

b8 mtVulkanMaterialShader::createShaderModule(const char *name, const char *typeStr,
                                              VkShaderStageFlagBits shaderStageFlag, u32 stageIndex,
                                              mtVulkanShaderStage *shaderStages) {

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
    shaderStages[stageIndex].createInfo.pCode = (u32 *)buffer.data();

    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();

    VK_CHECK(vkCreateShaderModule(device, &shaderStages[stageIndex].createInfo, 0, &shaderStages[stageIndex].handle));

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

    // Destroy object descriptor pool.
    vkDestroyDescriptorPool(device, _objectDescriptorPool, 0);

    // Destroy descriptor set layouts.
    vkDestroyDescriptorSetLayout(device, _objectDescriptorSetLayout, 0);
    // Destroy shader modules.
    for (u32 i = 0; i < SHADER_STAGE_COUNT; ++i) {
        vkDestroyShaderModule(device, _stages[i].handle, 0);
        _stages[i].handle = 0;
    }
}

void mtVulkanMaterialShader::use() {
    auto _pCommandBuffers = _context.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];
    _pipeline->bind(&commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS);
}

VkRenderPass mtVulkanMaterialShader::getRenderPassHandle() {
    return _context.getMainRenderPass()->getHandle();
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
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];
    VkCommandBuffer command_buffer = commandBuffer.getHandle();
    VkDescriptorSet global_descriptor = _globalDescriptorSets[image_index];
    vkCmdBindDescriptorSets(command_buffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(), 0, 1,
                            &global_descriptor, 0, 0);
}

void mtVulkanMaterialShader::updateObject(const mtRenderGeometry &geometry) {
    VkDevice device = _context.getVulkanDevice()->getLogicalDevice();
    u32 image_index = _context.getCurrentFrame();
    auto _pCommandBuffers = _context.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_context.getCurrentFrame()];

    vkCmdPushConstants(commandBuffer.getHandle(), _pipeline->getPipelineLayout(), VK_SHADER_STAGE_VERTEX_BIT, 0,
                       sizeof(glm::mat4), &geometry.geometry.model);

    VkDescriptorSet object_descriptor = _objectDescriptorSets[image_index];
    const u32 VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT = 2; // TODO: this should be dynamic based on the shader, but for
                                                         // now we can hardcode it since we only have one shader.
    VkWriteDescriptorSet descriptor_writes[VULKAN_OBJECT_SHADER_DESCRIPTOR_COUNT];
    u32 descriptor_count = 0;
    u32 descriptor_index = 0;

    mtObjectUniformObject object_ubo;
    object_ubo.diffuseColor = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

    object_uniform_buffer->loadData(0, sizeof(mtObjectUniformObject), 0, &object_ubo);

    VkDescriptorBufferInfo buffer_info;
    buffer_info.buffer = object_uniform_buffer->getHandle();
    buffer_info.offset = 0;
    buffer_info.range = sizeof(mtObjectUniformObject);

    VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
    descriptor.dstSet = object_descriptor;
    descriptor.dstBinding = descriptor_index;
    descriptor.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    descriptor.descriptorCount = 1;
    descriptor.pBufferInfo = &buffer_info;

    descriptor_writes[descriptor_count] = descriptor;
    descriptor_count++;

    descriptor_index++;

    const u32 sampler_count = 1;
    VkDescriptorImageInfo image_infos[1];

    if (geometry.texture->internalData != 0) {
        // MT_LOG_ERROR("Geometry does not have a texture assigned, but the shader expects one.");
        for (u32 sampler_index = 0; sampler_index < sampler_count; ++sampler_index) {

            mtTextureInternalData *internal_data = (mtTextureInternalData *)geometry.texture->internalData;

            // Assign view and sampler.
            image_infos[sampler_index].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            image_infos[sampler_index].imageView = internal_data->image->getImageView();
            image_infos[sampler_index].sampler = internal_data->sampler;

            VkWriteDescriptorSet descriptor = {VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET};
            descriptor.dstSet = object_descriptor;
            descriptor.dstBinding = descriptor_index;
            descriptor.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptor.descriptorCount = 1;
            descriptor.pImageInfo = &image_infos[sampler_index];

            descriptor_writes[descriptor_count] = descriptor;
            descriptor_count++;

            descriptor_index++;
        }
    }

    vkUpdateDescriptorSets(device, descriptor_count, descriptor_writes, 0, 0);
    vkCmdBindDescriptorSets(commandBuffer.getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, _pipeline->getPipelineLayout(),
                            1, 1, &object_descriptor, 0, 0);
}
