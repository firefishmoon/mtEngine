#pragma once

#include <vulkan/vulkan.h>
#include "defines.h"

class mtVulkanDevice;

class mtVulkanShader {
public:
    mtVulkanShader(mtVulkanDevice& device, const u32* bytes, u32 byteSize);

    ~mtVulkanShader();

private:
    VkShaderModule _handle = 0;
    VkShaderModuleCreateInfo _createInfo = {};
    VkPipelineShaderStageCreateInfo _shaderStageCreateInfo = {};
    mtVulkanDevice& _device;
};
