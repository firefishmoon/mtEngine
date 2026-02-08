#define GLFW_INCLUDE_VULKAN 
#include <GLFW/glfw3.h>
#include <vector>
#include <set>
#include <fstream>
#define VK_USE_PLATFORM_WIN32_KHR 
#include "vulkan_backend.h"
#include "core/loggersystem.h"
#include "core/application.h"
#include "core/platform.h"
#include "vulkan_error.h"
#include <windows.h>
#include <assert.h>

constexpr bool enableValidationLayers = true;
const std::vector<const char*> validationLayers = {
    "VK_LAYER_KHRONOS_validation"
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};



b8 mtVulkanBackend::initialize() {
    MT_LOG_INFO("Initializing Vulkan Backend");

    if (!_vulkanContext.initialize()) {
        MT_LOG_ERROR("Failed to initialize Vulkan context!");
        return false;
    }
    
    MAX_FRAMES_IN_FLIGHT = _vulkanContext.getVulkanSwapChain()->_imageCount;

    if (!createRenderPass()) {
        MT_LOG_ERROR("Failed to create render pass!");
        return false;
    }
    
    if (!createFramebuffers()) {
        MT_LOG_ERROR("Failed to create framebuffers!");
        return false;
    }
    
    if (!createCommandBuffers()) {
        MT_LOG_ERROR("Failed to create command buffers!");
        return false;
    }
    
    if (!createSyncObjects()) {
        MT_LOG_ERROR("Failed to create sync objects!");
        return false;
    }
    
    MT_LOG_INFO("Vulkan Backend Initialized");

    return true;
}

b8 mtVulkanBackend::shutdown() {
    // if (_device != VK_NULL_HANDLE) {
        vkDeviceWaitIdle(_vulkanContext._vulkanDevice._logicDevice);
    // }

    _inFlightFences.clear();
    _renderFinishedSemaphores.clear();
    _imageAvailableSemaphores.clear();

    _renderPass = VK_NULL_HANDLE;

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

b8 mtVulkanBackend::createSurface() {
    // mtPlatformData data = mtApplication::getInstance()->getPlatformData();
    // // VkResult result = glfwCreateWindowSurface(_vulkanContext._instance, data.window, 0, &_vulkanContext._surface);
    // VK_CHECK(glfwCreateWindowSurface(_vulkanContext._instance, data.window, 0, &_vulkanContext._surface));
    // if (result != VK_SUCCESS) {
    //     return false;
    // }
    // VkWin32SurfaceCreateInfoKHR createInfo = {};
    // createInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
    // createInfo.hwnd = data.hwnd;
    // createInfo.hinstance = data.hInstance;
    // if (vkCreateWin32SurfaceKHR(_vulkanContext._instance, &createInfo, nullptr, &_vulkanContext._surface) != VK_SUCCESS) {
    //     MT_LOG_FATAL("Failed to create window surface!");
    //     return false;
    // }
    // _surface = _vulkanContext._surface;
    return true;
}


b8 mtVulkanBackend::createRenderPass() {
    VkAttachmentDescription colorAttachment{};
    colorAttachment.format = _vulkanContext._vulkanSwapChain._imageFormat.format;
    colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
    colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
    colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
    colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
    colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
    colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
    colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

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
    dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

    VkRenderPassCreateInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
    renderPassInfo.attachmentCount = 1;
    renderPassInfo.pAttachments = &colorAttachment;
    renderPassInfo.subpassCount = 1;
    renderPassInfo.pSubpasses = &subpass;
    renderPassInfo.dependencyCount = 1;
    renderPassInfo.pDependencies = &dependency;

    VkDevice device = _vulkanContext._vulkanDevice._logicDevice;
    if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &_renderPass) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to create render pass");
        return false;
    }
    
    return true;
}

// b8 mtVulkanBackend::createGraphicsPipeline() {
//     auto vertShaderCode = readFile("engine/shaders/compiled/vert.spv");
//     auto fragShaderCode = readFile("engine/shaders/compiled/frag.spv");
//
//     VkShaderModule vertShaderModule = createShaderModule(vertShaderCode);
//     VkShaderModule fragShaderModule = createShaderModule(fragShaderCode);
//
//     VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
//     vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
//     vertShaderStageInfo.module = vertShaderModule;
//     vertShaderStageInfo.pName = "main";
//
//     VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
//     fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
//     fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
//     fragShaderStageInfo.module = fragShaderModule;
//     fragShaderStageInfo.pName = "main";
//
//     VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};
//
//     VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
//     vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
//     vertexInputInfo.vertexBindingDescriptionCount = 0;
//     vertexInputInfo.vertexAttributeDescriptionCount = 0;
//
//     VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
//     inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
//     inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
//     inputAssembly.primitiveRestartEnable = VK_FALSE;
//
//     VkViewport viewport{};
//     viewport.x = 0.0f;
//     viewport.y = 0.0f;
//     viewport.width = (float)_vulkanContext._width;
//     viewport.height = (float)_vulkanContext._height;
//     viewport.minDepth = 0.0f;
//     viewport.maxDepth = 1.0f;
//
//     VkRect2D scissor{};
//     scissor.offset = {0, 0};
//     scissor.extent = {_vulkanContext._width, _vulkanContext._height};
//
//     VkPipelineViewportStateCreateInfo viewportState{};
//     viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
//     viewportState.viewportCount = 1;
//     viewportState.pViewports = &viewport;
//     viewportState.scissorCount = 1;
//     viewportState.pScissors = &scissor;
//
//     VkPipelineRasterizationStateCreateInfo rasterizer{};
//     rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
//     rasterizer.depthClampEnable = VK_FALSE;
//     rasterizer.rasterizerDiscardEnable = VK_FALSE;
//     rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
//     rasterizer.lineWidth = 1.0f;
//     rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
//     rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
//     rasterizer.depthBiasEnable = VK_FALSE;
//
//     VkPipelineMultisampleStateCreateInfo multisampling{};
//     multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
//     multisampling.sampleShadingEnable = VK_FALSE;
//     multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
//
//     VkPipelineColorBlendAttachmentState colorBlendAttachment{};
//     colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
//     colorBlendAttachment.blendEnable = VK_FALSE;
//
//     VkPipelineColorBlendStateCreateInfo colorBlending{};
//     colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
//     colorBlending.logicOpEnable = VK_FALSE;
//     colorBlending.logicOp = VK_LOGIC_OP_COPY;
//     colorBlending.attachmentCount = 1;
//     colorBlending.pAttachments = &colorBlendAttachment;
//     colorBlending.blendConstants[0] = 0.0f;
//     colorBlending.blendConstants[1] = 0.0f;
//     colorBlending.blendConstants[2] = 0.0f;
//     colorBlending.blendConstants[3] = 0.0f;
//
//     VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
//     pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
//     pipelineLayoutInfo.setLayoutCount = 0;
//     pipelineLayoutInfo.pushConstantRangeCount = 0;
//
//     VkDevice device = _vulkanContext._vulkanDevice._logicDevice;
//     if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &_pipelineLayout) != VK_SUCCESS) {
//         MT_LOG_ERROR("Failed to create pipeline layout");
//         return false;
//     }
//
//     VkGraphicsPipelineCreateInfo pipelineInfo{};
//     pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
//     pipelineInfo.stageCount = 2;
//     pipelineInfo.pStages = shaderStages;
//     // pipelineInfo.stageCount = 0;
//     // pipelineInfo.pStages = 0;
//     pipelineInfo.pVertexInputState = &vertexInputInfo;
//     pipelineInfo.pInputAssemblyState = &inputAssembly;
//     pipelineInfo.pViewportState = &viewportState;
//     pipelineInfo.pRasterizationState = &rasterizer;
//     pipelineInfo.pMultisampleState = &multisampling;
//     pipelineInfo.pColorBlendState = &colorBlending;
//     pipelineInfo.layout = _pipelineLayout;
//     pipelineInfo.renderPass = _renderPass;
//     pipelineInfo.subpass = 0;
//     pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;
//     pipelineInfo.basePipelineIndex = -1;
//
//     if (vkCreateGraphicsPipelines(device, VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &_graphicsPipeline) != VK_SUCCESS) {
//         MT_LOG_ERROR("Failed to create graphics pipeline");
//         return false;
//     }
//
//     // destroy shader modules after pipeline creation
//     vkDestroyShaderModule(device, fragShaderModule, nullptr);
//     vkDestroyShaderModule(device, vertShaderModule, nullptr);
//
//     return true;
// }

b8 mtVulkanBackend::createFramebuffers() {
    _swapChainFramebuffers.resize(_vulkanContext._vulkanSwapChain._swapChainImageViews.size());

    for (size_t i = 0; i < _vulkanContext._vulkanSwapChain._swapChainImageViews.size(); i++) {
        VkImageView attachments[] = {_vulkanContext._vulkanSwapChain._swapChainImageViews[i]};

        VkFramebufferCreateInfo framebufferInfo{};
        framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
        framebufferInfo.renderPass = _renderPass;
        framebufferInfo.attachmentCount = 1;
        framebufferInfo.pAttachments = attachments;
        framebufferInfo.width = _vulkanContext._width;
        framebufferInfo.height = _vulkanContext._height;
        framebufferInfo.layers = 1;

        if (vkCreateFramebuffer(_vulkanContext._vulkanDevice._logicDevice, &framebufferInfo, nullptr, &_swapChainFramebuffers[i]) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to create framebuffer");
            return false;
        }
    }

    return true;
}


b8 mtVulkanBackend::createCommandBuffers() {
    // _commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);
    //
    // VkCommandBufferAllocateInfo allocInfo{};
    // allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
    // allocInfo.commandPool = _commandPool;
    // allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    // allocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;
    //
    // if (vkAllocateCommandBuffers(_device, &allocInfo, _commandBuffers.data()) != VK_SUCCESS) {
    //     MT_LOG_ERROR("Failed to allocate command buffers");
    //     return false;
    // }
    u32 imageCount = _vulkanContext.getVulkanSwapChain()->_imageCount;
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    _pCommandBuffers->resize(imageCount);
    for (auto& cmdBuffer : *_pCommandBuffers) {
        cmdBuffer.initialize(&_vulkanContext, true); 
    }

    return true;
}

b8 mtVulkanBackend::createSyncObjects() {
    _imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
    _inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

    VkSemaphoreCreateInfo semaphoreInfo{};
    semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
    VkFenceCreateInfo fenceInfo{};
    fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
    fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

    VkDevice device = _vulkanContext._vulkanDevice._logicDevice;
    for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
        if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_imageAvailableSemaphores[i]) != VK_SUCCESS ||
            vkCreateSemaphore(device, &semaphoreInfo, nullptr, &_renderFinishedSemaphores[i]) != VK_SUCCESS ||
            vkCreateFence(device, &fenceInfo, nullptr, &_inFlightFences[i]) != VK_SUCCESS) {
            MT_LOG_ERROR("Failed to create sync objects");
            return false;
        }
    }

    return true;
}

b8 mtVulkanBackend::recordCommandBuffer(VkCommandBuffer commandBuffer, u32 imageIndex) {
    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to begin recording command buffer!");
        return false;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _swapChainFramebuffers[imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {_vulkanContext._width, _vulkanContext._height};

    VkClearValue clearColor = {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);

    // vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, _graphicsPipeline);

    // VkViewport viewport{};
    // viewport.x = 0.0f;
    // viewport.y = 0.0f;
    // viewport.width = (float)_vulkanContext._width;
    // viewport.height = (float)_vulkanContext._height;
    // viewport.minDepth = 0.0f;
    // viewport.maxDepth = 1.0f;
    // vkCmdSetViewport(commandBuffer, 0, 1, &viewport);
    //
    // VkRect2D scissor{};
    // scissor.offset = {0, 0};
    // scissor.extent = {_vulkanContext._width, _vulkanContext._height};
    // vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);

    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to record command buffer!");
        return false;
    }

    return true;
}

b8 mtVulkanBackend::renderFrame() {
    VkDevice device = _vulkanContext._vulkanDevice._logicDevice;
    VkSwapchainKHR swapChain = _vulkanContext._vulkanSwapChain._handler;

    vkWaitForFences(device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);
    
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    VkCommandBuffer commandBuffer = (*_pCommandBuffers)[_currentFrame].getCommandBufferContext()->_commandBuffer;


    uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapChain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MT_LOG_ERROR("Failed to acquire swap chain image!");
        return false;
    }
    
    vkResetFences(device, 1, &_inFlightFences[_currentFrame]);

    vkResetCommandBuffer(commandBuffer, 0);

    if (!recordCommandBuffer(commandBuffer, imageIndex)) {
        return false;
    }
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(_vulkanContext._vulkanDevice._graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to submit draw command buffer!");
        return false;
    }
    
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &imageIndex;
    
    result = vkQueuePresentKHR(_vulkanContext._vulkanDevice._presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        return recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to present swap chain image!");
        return false;
    }
    
    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    
    return true;
}

b8 mtVulkanBackend::recreateSwapChain() {
    vkDeviceWaitIdle(_vulkanContext._vulkanDevice._logicDevice);

    if (!_vulkanContext.getVulkanSwapChain()->recreate(_vulkanContext._width, _vulkanContext._height)) {
        MT_LOG_ERROR("VulkanBackend recreateSwapChain fail.");
        return false;
    }
    
    createFramebuffers();
    
    _framebufferResized = false;
    MT_LOG_INFO("VulkanBackend recreateSwapChain success.");
    return true;
}

// b8 mtVulkanBackend::waitDeviceIdle() {
//     vkDeviceWaitIdle(_vulkanContext._vulkanDevice._logicDevice);
//     return true;
// }

// std::vector<char> mtVulkanBackend::readFile(const std::string& filename) {
//     std::ifstream file(filename, std::ios::ate | std::ios::binary);
//     
//     if (!file.is_open()) {
//         MT_LOG_ERROR("Failed to open file: %s", filename.c_str());
//         return {};
//     }
//     
//     size_t fileSize = (size_t)file.tellg();
//     std::vector<char> buffer(fileSize);
//     
//     file.seekg(0);
//     file.read(buffer.data(), fileSize);
//     file.close();
//     
//     return buffer;
// }
//
// VkShaderModule mtVulkanBackend::createShaderModule(const std::vector<char>& code) {
//     VkShaderModuleCreateInfo createInfo{};
//     createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
//     createInfo.codeSize = code.size();
//     createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
//
//     VkShaderModule shaderModule;
//     if (vkCreateShaderModule(_vulkanContext._vulkanDevice._logicDevice, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
//         MT_LOG_ERROR("Failed to create shader module");
//         return VK_NULL_HANDLE;
//     }
//
//     return shaderModule;
// }
//

b8 mtVulkanBackend::renderPrepare() {
    // If a resize was requested, recreate the swap chain before acquiring an image
    if (_framebufferResized) {
        // wait for device idle to ensure resources are not in use
        vkDeviceWaitIdle(_vulkanContext._vulkanDevice._logicDevice);

        // handle minimized window (width/height can be zero)
        if (_vulkanContext._width == 0 || _vulkanContext._height == 0) {
            return false; // skip rendering until non-zero
        }

        _framebufferResized = false;
        if (!recreateSwapChain()) {
            MT_LOG_ERROR("Failed to recreate swap chain during resize");
            return false;
        }
    }

    VkDevice device = _vulkanContext._vulkanDevice._logicDevice;
    VkSwapchainKHR swapChain = _vulkanContext._vulkanSwapChain._handler;

    vkWaitForFences(device, 1, &_inFlightFences[_currentFrame], VK_TRUE, UINT64_MAX);

    // uint32_t imageIndex;
    VkResult result = vkAcquireNextImageKHR(device, swapChain, UINT64_MAX, _imageAvailableSemaphores[_currentFrame], VK_NULL_HANDLE, &_vulkanContext._vulkanSwapChain._imageIndex);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR) {
        return recreateSwapChain();
    } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
        MT_LOG_ERROR("Failed to acquire swap chain image!");
        return false;
    }
    
    vkResetFences(device, 1, &_inFlightFences[_currentFrame]);

   
    return true;
}

b8 mtVulkanBackend::renderBegin() {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    VkCommandBuffer commandBuffer = (*_pCommandBuffers)[_currentFrame].getCommandBufferContext()->_commandBuffer;

    vkResetCommandBuffer(commandBuffer, 0);

    VkCommandBufferBeginInfo beginInfo{};
    beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
    beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

    if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to begin recording command buffer!");
        return false;
    }

    VkRenderPassBeginInfo renderPassInfo{};
    renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
    renderPassInfo.renderPass = _renderPass;
    renderPassInfo.framebuffer = _swapChainFramebuffers[_vulkanContext._vulkanSwapChain._imageIndex];
    renderPassInfo.renderArea.offset = {0, 0};
    renderPassInfo.renderArea.extent = {_vulkanContext._width, _vulkanContext._height};

    VkClearValue clearColor = {{{0.0f, 0.0f, 1.0f, 1.0f}}};
    renderPassInfo.clearValueCount = 1;
    renderPassInfo.pClearValues = &clearColor;

    vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
    return true;
}

b8 mtVulkanBackend::renderEnd() {
    auto _pCommandBuffers = _vulkanContext.getVulkanCommandBuffers();
    VkCommandBuffer commandBuffer = (*_pCommandBuffers)[_currentFrame].getCommandBufferContext()->_commandBuffer;

    VkViewport viewport{};
    viewport.x = 0.0f;
    viewport.y = 0.0f;
    viewport.width = (float)_vulkanContext._width;
    viewport.height = (float)_vulkanContext._height;
    viewport.minDepth = 0.0f;
    viewport.maxDepth = 1.0f;
    vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

    VkRect2D scissor{};
    scissor.offset = {0, 0};
    scissor.extent = {_vulkanContext._width, _vulkanContext._height};
    vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

    // vkCmdDraw(commandBuffer, 3, 1, 0, 0);

    vkCmdEndRenderPass(commandBuffer);


    if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to record command buffer!");
        return false;
    }
    
    VkSubmitInfo submitInfo{};
    submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
    
    VkSemaphore waitSemaphores[] = {_imageAvailableSemaphores[_currentFrame]};
    VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
    submitInfo.waitSemaphoreCount = 1;
    submitInfo.pWaitSemaphores = waitSemaphores;
    submitInfo.pWaitDstStageMask = waitStages;
    submitInfo.commandBufferCount = 1;
    submitInfo.pCommandBuffers = &commandBuffer;
    
    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    submitInfo.signalSemaphoreCount = 1;
    submitInfo.pSignalSemaphores = signalSemaphores;
    
    if (vkQueueSubmit(_vulkanContext._vulkanDevice._graphicsQueue, 1, &submitInfo, _inFlightFences[_currentFrame]) != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to submit draw command buffer!");
        return false;
    }

    return true;
}

b8 mtVulkanBackend::renderPresent() {
    VkSwapchainKHR swapChain = _vulkanContext._vulkanSwapChain._handler;

    VkSemaphore signalSemaphores[] = {_renderFinishedSemaphores[_currentFrame]};
    VkPresentInfoKHR presentInfo{};
    presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
    presentInfo.waitSemaphoreCount = 1;
    presentInfo.pWaitSemaphores = signalSemaphores;
    
    VkSwapchainKHR swapChains[] = {swapChain};
    presentInfo.swapchainCount = 1;
    presentInfo.pSwapchains = swapChains;
    presentInfo.pImageIndices = &_vulkanContext._vulkanSwapChain._imageIndex;
    
    VkResult result = vkQueuePresentKHR(_vulkanContext._vulkanDevice._presentQueue, &presentInfo);
    
    if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || _framebufferResized) {
        _framebufferResized = false;
        return recreateSwapChain();
    } else if (result != VK_SUCCESS) {
        MT_LOG_ERROR("Failed to present swap chain image!");
        return false;
    }
    
    _currentFrame = (_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    return true;
}
