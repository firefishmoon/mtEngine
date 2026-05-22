#pragma once

#include "defines.h"
#include "render/render_types.h"
#include "render/vulkan/vulkan_buffer.h"
#include <glm/glm.hpp>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#include <vulkan/vulkan.h>

class mtVulkanDevice;
class mtVulkanMaterialShader;

// Forward declare backend for frame count access
class mtVulkanBackend;

struct mtVulkanProgram {
    VkPipelineLayout pipelineLayout = VK_NULL_HANDLE;
    VkPipeline pipeline = VK_NULL_HANDLE;

    VkDescriptorSetLayout descriptorSetLayout = VK_NULL_HANDLE;
    VkDescriptorPool descriptorPool = VK_NULL_HANDLE;
    VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

    std::vector<VkDescriptorSetLayoutBinding> bindings;
    std::vector<VkDescriptorPoolSize> poolSizes;

    // Maps uniform name to its binding index
    std::unordered_map<std::string, u32> uniformNameToBinding;

    std::unique_ptr<mtVulkanBuffer> uniformBuffer;

    ~mtVulkanProgram() { shutdown(); }

    void shutdown();
};

struct mtVulkanShader {
    VkShaderModule module = VK_NULL_HANDLE;
    VkShaderStageFlagBits stage;
    u32 refCount = 0;
};

class mtVulkanProgramManager {
  public:
    mtVulkanProgramManager(mtVulkanDevice &device, mtVulkanMaterialShader &materialShader);
    ~mtVulkanProgramManager();

    // Create a shader module from SPIR-V bytes and return its handle
    VkShaderModule createShaderModule(const u8 *data, u32 dataSize, VkShaderStageFlagBits stage);
    void destroyShaderModule(VkShaderModule module);

    // Create a program from vertex/fragment shader modules
    b8 createProgram(VkShaderModule vertModule, VkShaderModule fragModule, mtProgramConfig &config, mtProgram &program);
    void destroyProgram(mtProgram &program);

    // Update a uniform on a program
    b8 updateUniform(mtProgram &program, const std::string &name, const void *data, u32 size);

    // Get the material shader (builtin)
    mtVulkanMaterialShader &getMaterialShader() { return _materialShader; }

  private:
    mtVulkanDevice &_device;
    mtVulkanMaterialShader &_materialShader;

    // Stored shader modules keyed by a simple ID
    // StoredShaderModule _shaderModules[MT_SHADER_MAX_COUNT] = {};
    // mtVulkanProgram _programs[MT_SHADER_MAX_COUNT] = {};
};
