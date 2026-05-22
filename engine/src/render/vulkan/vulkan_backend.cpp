#include "core/memorysystem.h"
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <set>
#include <vector>
#define VK_USE_PLATFORM_WIN32_KHR
#include "../render_types.h"
#include "core/loggersystem.h"
#include "vulkan_backend.h"
#include "vulkan_buffer.h"
#include "vulkan_program.h"
#include <assert.h>
#include <glm/glm.hpp>
#include <windows.h>

constexpr bool enableValidationLayers = true;
const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};

const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

b8 mtVulkanBackend::initialize(u32 width, u32 height) {
    MT_LOG_INFO("Initializing Vulkan Backend");

    if (!_vulkanContext.initialize(width, height)) {
        MT_LOG_ERROR("Failed to initialize Vulkan context!");
        return false;
    }

    MAX_FRAMES_IN_FLIGHT = _vulkanContext.getVulkanSwapChain()->getImageCount();

    // _vulkanContext.getMainRenderPass()->initialize(
    //     &_vulkanContext,
    //     0,
    //     0,
    //     _vulkanContext.getWidth(),
    //     _vulkanContext.getHeight(),
    //     0.0f,
    //     0.0f,
    //     0.2f,
    //     1.0f,
    //     1.0f,
    //     0.0f);

    // _vulkanContext.getMaterialShader().initialize(&_vulkanContext);

    if (!createFramebuffers()) {
        MT_LOG_ERROR("Failed to create framebuffers!");
        return false;
    }

    _programManager =
        std::make_unique<mtVulkanProgramManager>(*_vulkanContext.getVulkanDevice(), _vulkanContext.getMaterialShader());
    MAX_FRAMES_IN_FLIGHT = _vulkanContext.getVulkanSwapChain()->getImageCount();

    // test code
    const u32 vert_count = 4;
    mtVertex verts[vert_count];

    verts[0].position.x = -0.5f;
    verts[0].position.y = -0.5f;
    verts[0].position.z = 0.0f;
    verts[0].uv.x = 0.0f;
    verts[0].uv.y = 0.0f;

    verts[1].position.y = 0.5f;
    verts[1].position.x = 0.5f;
    verts[1].position.z = 0.0f;
    verts[1].uv.x = 1.0f;
    verts[1].uv.y = 1.0f;

    verts[2].position.x = -0.5f;
    verts[2].position.y = 0.5f;
    verts[2].position.z = 0.0f;
    verts[2].uv.x = 0.0f;
    verts[2].uv.y = 1.0f;

    verts[3].position.x = 0.5f;
    verts[3].position.y = -0.5f;
    verts[3].position.z = 0.0f;
    verts[3].uv.x = 1.0f;
    verts[3].uv.y = 0.0f;

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};

    uploadDataRange(_vulkanContext.getVulkanDevice()->getGraphicsCommandPool(), 0,
                    _vulkanContext.getVulkanDevice()->getGraphicsQueue(), _vulkanContext.getVertexBuffer(), 0,
                    sizeof(mtVertex) * vert_count, verts);

    MT_LOG_INFO("Vertex buffer uploaded with {} vertices", vert_count);

    uploadDataRange(_vulkanContext.getVulkanDevice()->getGraphicsCommandPool(), 0,
                    _vulkanContext.getVulkanDevice()->getGraphicsQueue(), _vulkanContext.getIndexBuffer(), 0,
                    sizeof(u32) * index_count, indices);

    MT_LOG_INFO("Index buffer uploaded with {} indices", index_count);

    MT_LOG_INFO("Vulkan Backend Initialized");

    return true;
}

b8 mtVulkanBackend::shutdown() {
    vkDeviceWaitIdle(_vulkanContext.getVulkanDevice()->getLogicalDevice());
    // _vulkanContext.getMainRenderPass()->shutdown();
    // _inFlightFences.clear();
    // _renderFinishedSemaphores.clear();
    // _imageAvailableSemaphores.clear();

    // _vulkanContext.shutdown();
    // _renderPass = VK_NULL_HANDLE;

    MT_LOG_INFO("Vulkan Backend shutdown");
    return true;
}

b8 mtVulkanBackend::checkDeviceExtensionSupport(VkPhysicalDevice device) {
    uint32_t extensionCount;
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

    std::vector<VkExtensionProperties> availableExtensions(extensionCount);
    vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

    std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

    for (const auto &extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}

b8 mtVulkanBackend::createFramebuffers() {
    // _swapChainFramebuffers.resize(_vulkanContext._vulkanSwapChain._swapChainImageViews.size());

    for (size_t i = 0; i < _vulkanContext.getVulkanSwapChain()->getSwapChainImageViews().size(); i++) {
        mtVector<VkImageView> vector = {_vulkanContext.getVulkanSwapChain()->getSwapChainImageViews()[i],
                                        _vulkanContext.getVulkanSwapChain()->getDepthAttachment().getImageView()};

        _vulkanContext.getVulkanSwapChain()->getFramebuffers()[i].initialize(
            &_vulkanContext, _vulkanContext.getMainRenderPass(), _vulkanContext.getWidth(), _vulkanContext.getHeight(),
            vector);
    }

    return true;
}

b8 mtVulkanBackend::recreateSwapChain() {
    vkDeviceWaitIdle(_vulkanContext.getVulkanDevice()->getLogicalDevice());

    if (!_vulkanContext.getVulkanSwapChain()->recreate(_vulkanContext.getWidth(), _vulkanContext.getHeight())) {
        MT_LOG_ERROR("VulkanBackend recreateSwapChain fail.");
        return false;
    }
    for (u32 i = 0; i < _vulkanContext.getVulkanSwapChain()->getFramebuffers().size(); ++i) {
        _vulkanContext.getVulkanSwapChain()->getFramebuffers()[i].shutdown();
    }

    createFramebuffers();

    _framebufferResized = false;
    MT_LOG_INFO("VulkanBackend recreateSwapChain success.");
    return true;
}

b8 mtVulkanBackend::renderPrepare() {
    // If a resize was requested, recreate the swap chain before acquiring an image
    if (_framebufferResized) {
        vkDeviceWaitIdle(_vulkanContext.getVulkanDevice()->getLogicalDevice());

        if (_vulkanContext.getWidth() == 0 || _vulkanContext.getHeight() == 0) {
            return false;
        }

        _framebufferResized = false;
        if (!recreateSwapChain()) {
            MT_LOG_ERROR("Failed to recreate swap chain during resize");
            return false;
        }
    }

    VkDevice device = _vulkanContext.getVulkanDevice()->getLogicalDevice();
    VkSwapchainKHR swapChain = _vulkanContext.getVulkanSwapChain()->getHandle();

    vkWaitForFences(device, 1, &_vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()], VK_TRUE,
                    UINT64_MAX);

    u32 imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(
        device, swapChain, UINT64_MAX, _vulkanContext.getImageAvailableSemaphores()[_vulkanContext.getCurrentFrame()],
        VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapChain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MT_LOG_ERROR("Failed to acquire swap chain image!");
        return false;
    }

    vkResetFences(device, 1, &_vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()]);

    _vulkanContext.getVulkanSwapChain()->setImageIndex(imageIndex);

    // Keep device aware of current swapchain dimensions
    _vulkanContext.getVulkanDevice()->setSwapChainDimensions(_vulkanContext.getWidth(), _vulkanContext.getHeight());

    return true;
}

b8 mtVulkanBackend::renderBegin() {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    commandBuffer.begin();
    mtVulkanFrameBuffer &frameBuffer =
        _vulkanContext.getVulkanSwapChain()->getFramebuffers()[_vulkanContext.getVulkanSwapChain()->getImageIndex()];

    // Dynamic state
    VkViewport viewport;
    viewport.x = 0.0f;
    viewport.y = (f32)_vulkanContext.getHeight();
    viewport.width = (f32)_vulkanContext.getWidth();
    viewport.height = -(f32)_vulkanContext.getHeight();
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;

    // Scissor
    VkRect2D scissor;
    scissor.offset.x = scissor.offset.y = 0;
    scissor.extent.width = _vulkanContext.getWidth();
    scissor.extent.height = _vulkanContext.getHeight();

    vkCmdSetViewport(commandBuffer.getHandle(), 0, 1, &viewport);
    vkCmdSetScissor(commandBuffer.getHandle(), 0, 1, &scissor);

    _vulkanContext.getMainRenderPass()->setExtent(_vulkanContext.getWidth(), _vulkanContext.getHeight());

    _vulkanContext.getMainRenderPass()->begin(commandBuffer, frameBuffer);

    return true;
}

b8 mtVulkanBackend::renderEnd() {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    _vulkanContext.getMainRenderPass()->end(commandBuffer);

    commandBuffer.end();

    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

    VkSemaphore waitSemaphores[] = {_vulkanContext.getImageAvailableSemaphores()[_vulkanContext.getCurrentFrame()]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer cb = commandBuffer.getHandle();
    submitInfo.pCommandBuffers = &cb;

    VkSemaphore signalSemaphores[] = {_vulkanContext.getRenderFinishedSemaphores()[_vulkanContext.getCurrentFrame()]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;

    if (vkQueueSubmit(_vulkanContext.getVulkanDevice()->getGraphicsQueue(), 1, &submitInfo,
                      _vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()]) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to submit draw command buffer!");
        throw std::runtime_error("Failed to submit draw command buffer!");
        return false;
    }

    return true;
}

b8 mtVulkanBackend::renderPresent() {
    _vulkanContext.getVulkanSwapChain()->represent(&_vulkanContext);
    return true;
}

void mtVulkanBackend::uploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, mtVulkanBuffer &buffer,
                                      u64 offset, u64 size, void *data) {
    VkBufferUsageFlags flags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
    mtVulkanBuffer staging(*_vulkanContext.getVulkanDevice(), size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);
    // staging.initialize(&_vulkanContext, size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, flags, true);
    // Load the data into the staging buffer.
    staging.loadData(0, size, 0, data);

    // Perform the copy from staging to the device local buffer.
    staging.copyTo(pool, fence, queue, 0, buffer.getHandle(), offset, size);

    // Clean up the staging buffer.
    // staging.shutdown();
}

void mtVulkanBackend::updateGlobalState(glm::mat4 projection, glm::mat4 view, glm::vec3 viewPosition,
                                        glm::vec4 ambientColor, s32 mode) {

    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    // vulkan_object_shader_use(&context, &context.object_shader);
    mtVulkanMaterialShader &materialShader = _vulkanContext.getMaterialShader();
    materialShader.use();

    materialShader.setProjection(projection);
    materialShader.setView(view);

    // TODO: other ubo properties

    // vulkan_object_shader_update_global_state(&context, &context.object_shader);
    materialShader.updateGlobalState();
}

void mtVulkanBackend::updateObject(const mtRenderGeometry &geometry) {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer &commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {0};
    VkBuffer vertexBuffer = _vulkanContext.getVertexBuffer().getHandle();
    vkCmdBindVertexBuffers(commandBuffer.getHandle(), 0, 1, &vertexBuffer, (VkDeviceSize *)offsets);

    // Bind index buffer at offset.
    vkCmdBindIndexBuffer(commandBuffer.getHandle(), _vulkanContext.getIndexBuffer().getHandle(), 0,
                         VK_INDEX_TYPE_UINT32);

    // Check if geometry has a custom program
    if (geometry.program) {
        // Use custom program
        mtVulkanProgram *program = (mtVulkanProgram *)geometry.program->internalData;
        if (program && program->pipeline != VK_NULL_HANDLE) {
            // Bind custom pipeline
            vkCmdBindPipeline(commandBuffer.getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipeline);

            // Auto-update texture uniform if texture is present and program has a sampler binding
            // if (geometry.texture && program->uniformNameToBinding.count("u_DiffuseTexture") > 0) {
            //     mtTextureInternalData *internalData = (mtTextureInternalData *)geometry.texture->internalData;
            //     if (internalData) {
            //         VkDescriptorImageInfo imageInfo{};
            //         imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            //         imageInfo.imageView = internalData->image->getImageView();
            //         imageInfo.sampler = internalData->sampler;
            //         _programManager->updateUniform(*geometry.program, "u_DiffuseTexture", &imageInfo,
            //                                        sizeof(VkDescriptorImageInfo));
            //     }
            // }

            // Bind descriptor sets for custom program
            VkDescriptorSet sets[] = {program->descriptorSet};
            vkCmdBindDescriptorSets(commandBuffer.getHandle(), VK_PIPELINE_BIND_POINT_GRAPHICS, program->pipelineLayout,
                                    0, 1, sets, 0, nullptr);

            // Push model matrix
            vkCmdPushConstants(commandBuffer.getHandle(), program->pipelineLayout, VK_SHADER_STAGE_VERTEX_BIT, 0,
                               sizeof(glm::mat4), &geometry.geometry.model);
        }
    } else {
        // Use built-in material shader
        mtVulkanMaterialShader &materialShader = _vulkanContext.getMaterialShader();
        materialShader.updateObject(geometry);
        materialShader.use();
    }

    // Issue the draw.
    vkCmdDrawIndexed(commandBuffer.getHandle(), 6, 1, 0, 0, 0);
}

//
// Shader management
//
void mtVulkanBackend::createShader(const u8 *data, u32 dataSize, mtShaderType type, mtShader &shader) {
    MT_LOG_INFO("mtVulkanBackend::createShader");
    u32 stage = VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT;
    if (type == mtShaderType::FRAGMENT) {
        stage = VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    VkShaderModule module = _programManager->createShaderModule(data, dataSize, (VkShaderStageFlagBits)stage);

    if (module == VK_NULL_HANDLE) {
        return;
    }
    shader.internalData = mtMemorySystem::getInstance()->allocate(mtMemTag::RENDERING, sizeof(mtVulkanShader));
    mtVulkanShader *vulkanShader = (mtVulkanShader *)shader.internalData;
    vulkanShader->module = module;
    vulkanShader->stage = (VkShaderStageFlagBits)stage;
    return;
}

void mtVulkanBackend::destroyShader(mtShader &shader) {
    mtVulkanShader *vulkanShader = (mtVulkanShader *)shader.internalData;
    _programManager->destroyShaderModule(vulkanShader->module);
    mtMemorySystem::getInstance()->deallocate(vulkanShader);
    shader.internalData = 0;
}

//
// Program management
//
void mtVulkanBackend::createProgram(mtShader &vertexShader, mtShader &fragmentShader, mtProgramConfig &config,
                                    mtProgram &program) {
    VkShaderModule vertModule = VK_NULL_HANDLE;
    VkShaderModule fragModule = VK_NULL_HANDLE;

    vertModule = ((mtVulkanShader *)vertexShader.internalData)->module;
    fragModule = ((mtVulkanShader *)fragmentShader.internalData)->module;

    if (!_programManager->createProgram(vertModule, fragModule, config, program)) {
        MT_LOG_ERROR("Failed to create program");
    }
}

void mtVulkanBackend::destroyProgram(mtProgram &program) { _programManager->destroyProgram(program); }

//
// Uniform updates
//
void mtVulkanBackend::updateUniform(mtProgram &program, const std::string &name, const void *data, u32 size) {
    _programManager->updateUniform(program, name, data, size);
}

void mtVulkanBackend::createTexture(const u8 *pixels, mtTexture &texture) {

    texture.internalData = MT_ALLOCATE(mtMemTag::RENDERING, sizeof(mtTextureInternalData));
    mtTextureInternalData *internalData = (mtTextureInternalData *)texture.internalData;

    VkDeviceSize imageSize = texture.width * texture.height * texture.channelCount;
    VkFormat imageFormat = VK_FORMAT_R8G8B8A8_UNORM;

    VkBufferUsageFlags usage = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
    VkMemoryPropertyFlags properties = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;

    mtVulkanBuffer stagingBuffer(*_vulkanContext.getVulkanDevice(), imageSize, usage, properties, true);

    stagingBuffer.loadData(0, imageSize, 0, (void *)pixels);

    // MT_LOG_INFO("internalData: {}, {}", (long long)internalData->image, internalData->sampler);

    // internalData->image = std::make_unique<mtVulkanImage>(
    internalData->image = new mtVulkanImage(*_vulkanContext.getVulkanDevice(), texture.width, texture.height,
                                            imageFormat, VK_IMAGE_TILING_OPTIMAL,
                                            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT |
                                                VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT, 1);

    // mtVulkanImage textureImage(
    //     *_vulkanContext.getVulkanDevice(),
    //     width,
    //     height,
    //     imageFormat,
    //     VK_IMAGE_TILING_OPTIMAL,
    //     VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT |
    //     VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, true, VK_IMAGE_ASPECT_COLOR_BIT,
    //     1);

    mtVulkanCommandBuffer tempBuffer(*_vulkanContext.getVulkanDevice(),
                                     _vulkanContext.getVulkanDevice()->getGraphicsCommandPool());
    tempBuffer.begin();

    // Transition the image layout to be optimal for receiving data.
    internalData->image->transitionLayout(tempBuffer.getHandle(), VK_IMAGE_LAYOUT_UNDEFINED,
                                          VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);

    // Copy the data from the staging buffer to the image.
    internalData->image->copyFromBuffer(tempBuffer.getHandle(), stagingBuffer.getHandle());

    // Transition the image layout to be optimal for shader access.
    internalData->image->transitionLayout(tempBuffer.getHandle(), VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                                          VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

    tempBuffer.end();

    // Submit the command buffer and wait for it to finish.
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    submitInfo.commandBufferCount = 1;
    VkCommandBuffer cb = tempBuffer.getHandle();
    submitInfo.pCommandBuffers = &cb;

    if (vkQueueSubmit(_vulkanContext.getVulkanDevice()->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE) !=
        VK_SUCCESS) {
        MT_LOG_ERROR("Failed to submit command buffer for texture upload!");
    }
    vkQueueWaitIdle(_vulkanContext.getVulkanDevice()->getGraphicsQueue());

    VkSamplerCreateInfo samplerInfo{};
    samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
    samplerInfo.magFilter = VK_FILTER_LINEAR;
    samplerInfo.minFilter = VK_FILTER_LINEAR;
    samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
    samplerInfo.anisotropyEnable = VK_TRUE;
    samplerInfo.maxAnisotropy = 16;
    samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
    samplerInfo.unnormalizedCoordinates = VK_FALSE;
    samplerInfo.compareEnable = VK_FALSE;
    samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
    samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
    samplerInfo.mipLodBias = 0.0f;
    samplerInfo.minLod = 0.0f;
    samplerInfo.maxLod = 0.0f;

    VkSampler textureSampler;
    if (vkCreateSampler(_vulkanContext.getVulkanDevice()->getLogicalDevice(), &samplerInfo, nullptr, &textureSampler) !=
        VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create texture sampler!");
    }

    internalData->sampler = textureSampler;
}

void mtVulkanBackend::destroyTexture(mtTexture &texture) {
    if (texture.internalData) {
        vkDeviceWaitIdle(_vulkanContext.getVulkanDevice()->getLogicalDevice());
        mtTextureInternalData *internalData = (mtTextureInternalData *)texture.internalData;
        delete internalData->image;
        vkDestroySampler(_vulkanContext.getVulkanDevice()->getLogicalDevice(), internalData->sampler, nullptr);
        MT_FREE(texture.internalData);
        texture.internalData = nullptr;
    }
}
