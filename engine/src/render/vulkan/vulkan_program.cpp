#include "vulkan_program.h"
#include "core/loggersystem.h"
#include "core/memorysystem.h"
#include "render/render_types.h"
#include "render/vulkan/shaders/vulkan_material_shader.h"
#include "vulkan_device.h"
#include "vulkan_error.h"
#include <cstring>

// In vulkan_program.cpp, use a local constant matching the backend
static const u32 MAX_FRAMES_IN_FLIGHT = 3;

mtVulkanProgramManager::mtVulkanProgramManager(mtVulkanDevice &device, mtVulkanMaterialShader &materialShader)
    : _device(device), _materialShader(materialShader) {}

mtVulkanProgramManager::~mtVulkanProgramManager() {}

VkShaderModule mtVulkanProgramManager::createShaderModule(const u8 *data, u32 dataSize, VkShaderStageFlagBits stage) {
    VkShaderModuleCreateInfo createInfo{};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.codeSize = dataSize;
    createInfo.pCode = reinterpret_cast<const u32 *>(data);

    VkDevice device = _device.getLogicalDevice();
    VkShaderModule module;
    VK_CHECK(vkCreateShaderModule(device, &createInfo, nullptr, &module));

    return module;
}

void mtVulkanProgramManager::destroyShaderModule(VkShaderModule module) {
    if (module == VK_NULL_HANDLE)
        return;

    vkDestroyShaderModule(_device.getLogicalDevice(), module, nullptr);
}

b8 mtVulkanProgramManager::createProgram(VkShaderModule vertModule, VkShaderModule fragModule, mtProgramConfig &config,
                                         mtProgram &program) {

    MT_LOG_INFO("mtVulkanProgramManager::createProgram");
    program.internalData = mtMemorySystem::getInstance()->allocate(mtMemTag::RENDERING, sizeof(mtVulkanProgram));
    mtVulkanProgram *vulkanProgram = new (program.internalData) mtVulkanProgram();
    VkDevice device = _device.getLogicalDevice();

    // Create shader stage infos
    std::vector<VkPipelineShaderStageCreateInfo> stageInfos;

    if (vertModule != VK_NULL_HANDLE) {
        VkPipelineShaderStageCreateInfo vertStage{};
        vertStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertStage.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertStage.module = vertModule;
        vertStage.pName = "main";
        stageInfos.push_back(vertStage);
    }

    if (fragModule != VK_NULL_HANDLE) {
        VkPipelineShaderStageCreateInfo fragStage{};
        fragStage.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragStage.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragStage.module = fragModule;
        fragStage.pName = "main";
        stageInfos.push_back(fragStage);
    }

    if (stageInfos.empty()) {
        MT_LOG_ERROR("No shader stages provided for program");
        return false;
    }

    // Build descriptor set layout from config uniforms
    for (auto &uniform : config.uniforms) {
        VkDescriptorSetLayoutBinding binding{};
        binding.binding = uniform.location;
        binding.descriptorCount = 1;
        binding.stageFlags = VK_SHADER_STAGE_ALL;

        switch (uniform.type) {
        case mtUniformType::SAMPER2D:
            binding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            break;
        default:
            binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            break;
        }

        vulkanProgram->bindings.push_back(binding);

        // Map uniform name to its binding index
        vulkanProgram->uniformNameToBinding[uniform.name] = uniform.location;

        // Build pool sizes
        bool found = false;
        for (auto &poolSize : vulkanProgram->poolSizes) {
            if (poolSize.type == binding.descriptorType) {
                poolSize.descriptorCount += 1;
                found = true;
                break;
            }
        }
        if (!found) {
            VkDescriptorPoolSize poolSize{};
            poolSize.type = binding.descriptorType;
            poolSize.descriptorCount = MAX_FRAMES_IN_FLIGHT;
            vulkanProgram->poolSizes.push_back(poolSize);
        }
    }

    VkDescriptorSetLayoutCreateInfo layoutInfo{};
    layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
    layoutInfo.bindingCount = (u32)vulkanProgram->bindings.size();
    layoutInfo.pBindings = vulkanProgram->bindings.data();

    VK_CHECK(vkCreateDescriptorSetLayout(device, &layoutInfo, nullptr, &vulkanProgram->descriptorSetLayout));

    // Create descriptor pool
    VkDescriptorPoolCreateInfo poolInfo{};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = (u32)vulkanProgram->poolSizes.size();
    poolInfo.pPoolSizes = vulkanProgram->poolSizes.data();
    poolInfo.maxSets = MAX_FRAMES_IN_FLIGHT;

    VK_CHECK(vkCreateDescriptorPool(device, &poolInfo, nullptr, &vulkanProgram->descriptorPool));

    // Allocate descriptor set
    VkDescriptorSetAllocateInfo allocInfo{};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = vulkanProgram->descriptorPool;
    allocInfo.descriptorSetCount = 1;
    allocInfo.pSetLayouts = &vulkanProgram->descriptorSetLayout;

    VK_CHECK(vkAllocateDescriptorSets(device, &allocInfo, &vulkanProgram->descriptorSet));

    // Create uniform buffer for this program
    VkDeviceSize bufferSize = sizeof(mtObjectUniformObject);
    vulkanProgram->uniformBuffer = std::make_unique<mtVulkanBuffer>(
        _device, bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
        VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT |
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
        true);

    // Create pipeline layout
    VkPushConstantRange pushConstant{};
    pushConstant.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
    pushConstant.offset = 0;
    pushConstant.size = sizeof(glm::mat4) * 2; // model + reserved

    VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
    pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
    pipelineLayoutInfo.setLayoutCount = 1;
    pipelineLayoutInfo.pSetLayouts = &vulkanProgram->descriptorSetLayout;
    pipelineLayoutInfo.pushConstantRangeCount = 1;
    pipelineLayoutInfo.pPushConstantRanges = &pushConstant;

    VK_CHECK(vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &vulkanProgram->pipelineLayout));

    // Get viewport/scissor from swapchain
    u32 width = _device.getSwapChainWidth();
    u32 height = _device.getSwapChainHeight();

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = (f32)height;
    viewport.width = (f32)width;
    viewport.height = -(f32)height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {width, height};

    // Vertex input state
    VkVertexInputBindingDescription bindingDesc{};
    bindingDesc.binding = 0;
    bindingDesc.stride = sizeof(mtVertex);
    bindingDesc.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    VkVertexInputAttributeDescription attributeDescs[2];
    attributeDescs[0].binding = 0;
    attributeDescs[0].location = 0;
    attributeDescs[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescs[0].offset = 0;
    attributeDescs[1].binding = 0;
    attributeDescs[1].location = 1;
    attributeDescs[1].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescs[1].offset = sizeof(glm::vec3);

    VkPipelineVertexInputStateCreateInfo vertexInput{};
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributeDescs;

    // Build pipeline
    VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
    inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
    inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
    inputAssembly.primitiveRestartEnable = VK_FALSE;

    VkPipelineViewportStateCreateInfo viewportState{};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.viewportCount = 1;
    viewportState.pViewports = &viewport;
    viewportState.scissorCount = 1;
    viewportState.pScissors = &scissor;

    VkPipelineRasterizationStateCreateInfo rasterizer{};
    rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
    rasterizer.depthClampEnable = VK_FALSE;
    rasterizer.rasterizerDiscardEnable = VK_FALSE;
    rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
    rasterizer.lineWidth = 1.0f;
    rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
    rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
    rasterizer.depthBiasEnable = VK_FALSE;

    VkPipelineMultisampleStateCreateInfo multisampling{};
    multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
    multisampling.sampleShadingEnable = VK_FALSE;
    multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencil{};
    depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
    depthStencil.depthTestEnable = VK_TRUE;
    depthStencil.depthWriteEnable = VK_TRUE;
    depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
    depthStencil.stencilTestEnable = VK_FALSE;

    VkPipelineColorBlendAttachmentState blendAttachment{};
    blendAttachment.blendEnable = VK_TRUE;
    blendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
    blendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
    blendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    blendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

    VkPipelineColorBlendStateCreateInfo blendState{};
    blendState.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    blendState.attachmentCount = 1;
    blendState.pAttachments = &blendAttachment;

    const VkDynamicState dynamicStates[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
    VkPipelineDynamicStateCreateInfo dynamicState{};
    dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicState.dynamicStateCount = 2;
    dynamicState.pDynamicStates = dynamicStates;

    VkGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    pipelineInfo.stageCount = (u32)stageInfos.size();
    pipelineInfo.pStages = stageInfos.data();
    pipelineInfo.pVertexInputState = &vertexInput;
    pipelineInfo.pInputAssemblyState = &inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vulkanProgram->pipelineLayout;
    pipelineInfo.renderPass = _materialShader.getRenderPassHandle();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributeDescs;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &rasterizer;
    pipelineInfo.pMultisampleState = &multisampling;
    pipelineInfo.pDepthStencilState = &depthStencil;
    pipelineInfo.pColorBlendState = &blendState;
    pipelineInfo.pDynamicState = &dynamicState;
    pipelineInfo.layout = vulkanProgram->pipelineLayout;
    pipelineInfo.renderPass = _materialShader.getRenderPassHandle();
    pipelineInfo.subpass = 0;
    pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
    pipelineInfo.basePipelineIndex = -1;

    // Temporarily use the built-in pipeline's vertex input state
    vertexInput.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
    vertexInput.vertexBindingDescriptionCount = 1;
    vertexInput.pVertexBindingDescriptions = &bindingDesc;
    vertexInput.vertexAttributeDescriptionCount = 2;
    vertexInput.pVertexAttributeDescriptions = attributeDescs;

    MT_LOG_INFO("mtVulkanProgramManager::createProgram 4");
    VkResult result =
        vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &vulkanProgram->pipeline);
    if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create pipeline for custom program: {}", (s32)result);
        return false;
    }

    return true;
}

void mtVulkanProgramManager::destroyProgram(mtProgram &program) {

    mtVulkanProgram *vulkanProgram = (mtVulkanProgram *)program.internalData;
    if (vulkanProgram->pipeline == VK_NULL_HANDLE)
        return;

    VkDevice device = _device.getLogicalDevice();

    if (vulkanProgram->pipeline != VK_NULL_HANDLE) {
        vkDestroyPipeline(device, vulkanProgram->pipeline, nullptr);
        vulkanProgram->pipeline = VK_NULL_HANDLE;
    }
    if (vulkanProgram->pipelineLayout != VK_NULL_HANDLE) {
        vkDestroyPipelineLayout(device, vulkanProgram->pipelineLayout, nullptr);
        vulkanProgram->pipelineLayout = VK_NULL_HANDLE;
    }
    if (vulkanProgram->descriptorSetLayout != VK_NULL_HANDLE) {
        vkDestroyDescriptorSetLayout(device, vulkanProgram->descriptorSetLayout, nullptr);
        vulkanProgram->descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (vulkanProgram->descriptorPool != VK_NULL_HANDLE) {
        vkDestroyDescriptorPool(device, vulkanProgram->descriptorPool, nullptr);
        vulkanProgram->descriptorPool = VK_NULL_HANDLE;
    }
    vulkanProgram->uniformBuffer.reset();
    vulkanProgram->bindings.clear();
    vulkanProgram->poolSizes.clear();
}

void mtVulkanProgram::shutdown() {
    if (pipeline != VK_NULL_HANDLE) {
        // Note: device is not stored, so we leak the pipeline here
        // In a real implementation, we'd need the device reference
        pipeline = VK_NULL_HANDLE;
    }
    if (pipelineLayout != VK_NULL_HANDLE) {
        pipelineLayout = VK_NULL_HANDLE;
    }
    if (descriptorSetLayout != VK_NULL_HANDLE) {
        descriptorSetLayout = VK_NULL_HANDLE;
    }
    if (descriptorPool != VK_NULL_HANDLE) {
        descriptorPool = VK_NULL_HANDLE;
    }
    uniformBuffer.reset();
    bindings.clear();
    poolSizes.clear();
}

// mtVulkanProgram *mtVulkanProgramManager::getProgram(mtProgramHandle handle) {
//     if (handle.id >= MT_SHADER_MAX_COUNT)
//         return nullptr;
//     if (_programs[handle.id].pipeline == VK_NULL_HANDLE)
//         return nullptr;
//     return &_programs[handle.id];
// }

b8 mtVulkanProgramManager::updateUniform(mtProgram &program, const std::string &name, const void *data, u32 size) {
    mtVulkanProgram *vulkanProgram = (mtVulkanProgram *)program.internalData;
    if (!vulkanProgram) {
        MT_LOG_ERROR("Invalid program handle for uniform update: '{}'", name);
        return false;
    }

    // Look up the binding index by uniform name
    auto it = vulkanProgram->uniformNameToBinding.find(name);
    if (it == vulkanProgram->uniformNameToBinding.end()) {
        return false;
    }

    u32 bindingIndex = it->second;

    // Find the matching binding descriptor
    for (auto &binding : vulkanProgram->bindings) {
        if (binding.binding == bindingIndex) {
            if (binding.descriptorType == VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER) {
                // Update uniform buffer
                if (vulkanProgram->uniformBuffer) {
                    vulkanProgram->uniformBuffer->loadData(0, size, 0, data);

                    VkDescriptorBufferInfo bufferInfo{};
                    bufferInfo.buffer = vulkanProgram->uniformBuffer->getHandle();
                    bufferInfo.offset = 0;
                    bufferInfo.range = size;

                    VkWriteDescriptorSet descriptorWrite{};
                    descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                    descriptorWrite.dstSet = vulkanProgram->descriptorSet;
                    descriptorWrite.dstBinding = binding.binding;
                    descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
                    descriptorWrite.descriptorCount = 1;
                    descriptorWrite.pBufferInfo = &bufferInfo;

                    vkUpdateDescriptorSets(_device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
                    return true;
                }
            } else if (binding.descriptorType == VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER) {
                // For sampler bindings, data should be a VkDescriptorImageInfo
                const VkDescriptorImageInfo *imageInfo = static_cast<const VkDescriptorImageInfo *>(data);

                VkWriteDescriptorSet descriptorWrite{};
                descriptorWrite.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
                descriptorWrite.dstSet = vulkanProgram->descriptorSet;
                descriptorWrite.dstBinding = binding.binding;
                descriptorWrite.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
                descriptorWrite.descriptorCount = 1;
                descriptorWrite.pImageInfo = imageInfo;

                vkUpdateDescriptorSets(_device.getLogicalDevice(), 1, &descriptorWrite, 0, nullptr);
                return true;
            }
        }
    }

    return false;
}
