#pragma once

#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include <memory>

#include "defines.h"
#include "render/vulkan/vulkan_instance.h"
#include "render/vulkan/vulkan_device.h"
#include "render/vulkan/vulkan_swapchain.h"
#include "render/vulkan/vulkan_command_buffer.h"
#include "render/vulkan/vulkan_renderpass.h"
#include "render/vulkan/vulkan_buffer.h"
#include "render/vulkan/shaders/vulkan_material_shader.h"
#include "core/std_wrapper.h"


class mtVulkanContext {
public:
    mtVulkanContext();
    ~mtVulkanContext();

    b8 initialize(u32 width, u32 height);
    b8 shutdown();

    // VkInstance getInstance() const { return _instance; }
    // mtVkContext* getVkContext() { return &_context; }
    inline mtVulkanDevice* getVulkanDevice() { return _vulkanDevice.get(); }
    inline mtVulkanSwapChain* getVulkanSwapChain() { return &_vulkanSwapChain; }
    inline mtVector<mtVulkanCommandBuffer>* getVulkanCommandBuffers() { return &_commandBuffers; }

    // Accessors to reduce direct member access from friends
    inline VkSurfaceKHR getSurface() const { return _surface; }
    inline VkInstance getInstance() const { return _instance->getHandle(); }
    // inline b8 isDebugEnabled() const { return _enableDebug; }
    inline u16 getWidth() const { return _width; }
    inline u16 getHeight() const { return _height; }
    inline u32 getCurrentFrame() const { return _currentFrame; }
    void setCurrentFrame(u32 frame) { _currentFrame = frame; }
    void setWidth(u16 w) { _width = w; }
    void setHeight(u16 h) { _height = h; }

    inline mtVector<VkSemaphore>& getImageAvailableSemaphores() { return _imageAvailableSemaphores; }
    inline mtVector<VkSemaphore>& getRenderFinishedSemaphores() { return _renderFinishedSemaphores; }
    inline mtVector<VkFence>& getInFlightFences() { return _inFlightFences; }
    inline mtVulkanRenderPass* getMainRenderPass() { return &_mainRenderPass; }

    inline mtVulkanBuffer& getVertexBuffer() { return _vertexBuffer; }
    inline mtVulkanBuffer& getIndexBuffer() { return _indexBuffer; }
    inline mtVulkanMaterialShader& getMaterialShader() { return _objectShader; }

protected:
    b8 createBuffers();

    std::unique_ptr<mtVulkanInstance> _instance;
    // friend class mtVulkanDevice;
    // mtVkContext _context;

    std::unique_ptr<mtVulkanDevice> _vulkanDevice;

    // mtVulkanDevice _vulkanDevice;
    mtVulkanSwapChain _vulkanSwapChain;
    mtVector<mtVulkanCommandBuffer> _commandBuffers;

    u32 _apiMajor;
    u32 _apiMinor;
    u32 _apiPatch;

    // b8 _enableDebug;
    //
    // VkInstance _instance;
    // VkDebugUtilsMessengerEXT _debugMessenger;

    VkSurfaceKHR _surface;

    u32 _width;
    u32 _height;

    mtVector<VkSemaphore> _imageAvailableSemaphores;
    mtVector<VkSemaphore> _renderFinishedSemaphores;
    mtVector<VkFence> _inFlightFences;

    mtVulkanRenderPass _mainRenderPass;

    mtVulkanBuffer _vertexBuffer;
    mtVulkanBuffer _indexBuffer;

    u32 _currentFrame;

    mtVulkanMaterialShader _objectShader;
};
