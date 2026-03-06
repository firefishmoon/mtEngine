#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#define VK_USE_PLATFORM_WIN32_KHR
#include "vulkan_backend.h"
#include "core/loggersystem.h"
#include "vulkan_buffer.h"
#include <windows.h>
#include <assert.h>
#include <glm/glm.hpp>

constexpr bool enableValidationLayers = true;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



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

    // test code
    const u32 vert_count = 4;
    glm::vec3 verts[vert_count];

    verts[0].x = -0.5f;
    verts[0].y = -0.5f;
    verts[0].z = 0.0f;

    verts[1].y = 0.5f;
    verts[1].x = 0.5f;
    verts[1].z = 0.0f;

    verts[2].x = -0.5f;
    verts[2].y = 0.5f;
    verts[2].z = 0.0f;

    verts[3].x = 0.5f;
    verts[3].y = -0.5f;
    verts[3].z = 0.0f;

    const u32 index_count = 6;
    u32 indices[index_count] = {0, 1, 2, 0, 3, 1};

    uploadDataRange(
        _vulkanContext.getVulkanDevice()->getGraphicsCommandPool(),
        0,
        _vulkanContext.getVulkanDevice()->getGraphicsQueue(),
        _vulkanContext.getVertexBuffer(),
        0,
        sizeof(glm::vec3) * vert_count,
        verts);

    MT_LOG_INFO("Vertex buffer uploaded with {} vertices", vert_count);

    uploadDataRange(
        _vulkanContext.getVulkanDevice()->getGraphicsCommandPool(),
        0,
        _vulkanContext.getVulkanDevice()->getGraphicsQueue(),
        _vulkanContext.getIndexBuffer(),
        0,
        sizeof(u32) * index_count,
        indices);

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

    for (const auto& extension : availableExtensions) {
        requiredExtensions.erase(extension.extensionName);
    }

    return requiredExtensions.empty();
}


b8 mtVulkanBackend::createFramebuffers() {
    // _swapChainFramebuffers.resize(_vulkanContext._vulkanSwapChain._swapChainImageViews.size());

    for (size_t i = 0; i < _vulkanContext.getVulkanSwapChain()->getSwapChainImageViews().size(); i++) {
        mtVector<VkImageView> vector = {
            _vulkanContext.getVulkanSwapChain()->getSwapChainImageViews()[i],
            _vulkanContext.getVulkanSwapChain()->getDepthAttachment().getImageView()
        };

        _vulkanContext.getVulkanSwapChain()->getFramebuffers()[i].initialize(
            &_vulkanContext,
            _vulkanContext.getMainRenderPass(),
            _vulkanContext.getWidth(),
            _vulkanContext.getHeight(),
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

    vkWaitForFences(device, 1, &_vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()], VK_TRUE, UINT64_MAX);

    u32 imageIndex = 0;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _vulkanContext.getImageAvailableSemaphores()[_vulkanContext.getCurrentFrame()], VK_NULL_HANDLE, &imageIndex);
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapChain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MT_LOG_ERROR("Failed to acquire swap chain image!");
        return false;
    }

    vkResetFences(device, 1, &_vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()]);

    _vulkanContext.getVulkanSwapChain()->setImageIndex(imageIndex);

    return true;
}

b8 mtVulkanBackend::renderBegin() {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    commandBuffer.begin();
    mtVulkanFrameBuffer& frameBuffer = _vulkanContext.getVulkanSwapChain()->getFramebuffers()[_vulkanContext.getVulkanSwapChain()->getImageIndex()];

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
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

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

    if (vkQueueSubmit(_vulkanContext.getVulkanDevice()->getGraphicsQueue(), 1, &submitInfo, _vulkanContext.getInFlightFences()[_vulkanContext.getCurrentFrame()]) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to submit draw command buffer!");
        return false;
    }

    return true;
}

b8 mtVulkanBackend::renderPresent() {
    _vulkanContext.getVulkanSwapChain()->represent(&_vulkanContext);
    return true;
}

void mtVulkanBackend::uploadDataRange(VkCommandPool pool, VkFence fence, VkQueue queue, mtVulkanBuffer& buffer, u64 offset, u64 size, void* data) {
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

void mtVulkanBackend::updateGlobalState(
        glm::mat4 projection,
        glm::mat4 view,
        glm::vec3 viewPosition,
        glm::vec4 ambientColor,
        s32 mode
    ) {

    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];

    // vulkan_object_shader_use(&context, &context.object_shader);
    mtVulkanMaterialShader& materialShader = _vulkanContext.getMaterialShader();
    materialShader.use();

    materialShader.setProjection(projection);
    materialShader.setView(view);

    // TODO: other ubo properties

    // vulkan_object_shader_update_global_state(&context, &context.object_shader);
    materialShader.updateGlobalState();

}

void mtVulkanBackend::updateObject(glm::mat4 model) {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    mtVulkanCommandBuffer& commandBuffer = *(*_pCommandBuffers)[_vulkanContext.getCurrentFrame()];
    mtVulkanMaterialShader& materialShader = _vulkanContext.getMaterialShader();

    materialShader.updateObject(model);

    // TODO: temporary test code
    materialShader.use();

    // Bind vertex buffer at offset.
    VkDeviceSize offsets[1] = {0};
    VkBuffer vertexBuffer = _vulkanContext.getVertexBuffer().getHandle();
    vkCmdBindVertexBuffers(commandBuffer.getHandle(), 0, 1, &vertexBuffer, (VkDeviceSize*)offsets);

    // Bind index buffer at offset.
    vkCmdBindIndexBuffer(commandBuffer.getHandle(), _vulkanContext.getIndexBuffer().getHandle(), 0, VK_INDEX_TYPE_UINT32);

    // Issue the draw.
    vkCmdDrawIndexed(commandBuffer.getHandle(), 6, 1, 0, 0, 0);

}
