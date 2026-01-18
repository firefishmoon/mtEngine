#include "rendersystem.h"
#include "vulkan/vulkan_backend.h"
#include "core/loggersystem.h"
#include "core/memorysystem.h"

template<> MT_API mtRenderSystem* Singleton<mtRenderSystem>::_instance = nullptr;

b8 mtRenderSystem::initialize() {
    // Initialize the rendering backend based on settings
    MT_LOG_INFO("Initializing Render System with API: {}", static_cast<int>(_settings.api));
    switch (_settings.api) {
        case mtBackendAPI::OPENGL:
            // Initialize OpenGL backend
            break;
        case mtBackendAPI::VULKAN:
            // Initialize Vulkan backend
            _backend = MT_NEW(mtMemTag::RENDERING, mtVulkanBackend);
            break;
        case mtBackendAPI::DIRECTX12:
            // Initialize DirectX 12 backend
            break;
        case mtBackendAPI::METAL:
            // Initialize Metal backend
            break;
        default:
            return false;
    }
    if (_backend && !_backend->initialize()) {
        MT_LOG_ERROR("Failed to initialize rendering backend");
        return false;
    }
    MT_LOG_INFO("Render System Initialized");
    return true;
}

b8 mtRenderSystem::shutdown() {
    // Shutdown the rendering backend
    if (_backend)
        delete _backend;
    return true;
}

void mtRenderSystem::renderFrame(const mtRenderPacket& packet) {
    // Process the render packet and issue draw calls
}
