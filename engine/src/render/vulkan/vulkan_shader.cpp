#include "render/vulkan/vulkan_shader.h"
#include "render/vulkan/vulkan_device.h"
#include "render/vulkan/vulkan_error.h"

mtVulkanShader::mtVulkanShader(mtVulkanDevice& device, const u32* bytes, u32 byteSize) : _device(device) {
    _createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    _createInfo.codeSize = byteSize;
    _createInfo.pCode = (u32 *)bytes;
    VK_CHECK(vkCreateShaderModule(device.getLogicalDevice(), &_createInfo, 0, &_handle));
}

mtVulkanShader::~mtVulkanShader() {
    vkDestroyShaderModule(_device.getLogicalDevice(), _handle, 0);
    _handle = 0;
}
