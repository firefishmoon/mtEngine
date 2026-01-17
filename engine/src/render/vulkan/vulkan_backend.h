#include "../../defines.h"
#include "../ibackend.h"
#include <vulkan/vulkan_raii.hpp>

class MT_API mtVulkanBackend : public mtIBackend {
public:
    mtVulkanBackend() = default;
    ~mtVulkanBackend() override = default;

    b8 initialize() override;
    b8 shutdown() override;
private:
    vk::raii::Context _context;
    vk::raii::Instance _instance{nullptr};
};
