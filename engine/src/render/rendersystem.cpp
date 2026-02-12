#include "rendersystem.h"
#include "vulkan/vulkan_backend.h"
#include "core/loggersystem.h"
#include "core/memorysystem.h"
#include "core/eventsystem.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

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
    if (_backend && !_backend->initialize(_settings.width, _settings.height)) {
        MT_LOG_ERROR("Failed to initialize rendering backend");
        return false;
    }
    mtEventSystem::getInstance()->registerEvent(mtEventType::FRAME, [this](mtEvent event) {
        this->renderFrame({event.fdata});
        
    });
    mtEventSystem::getInstance()->registerEvent(mtEventType::WINDOW_RESIZE, [this](mtEvent event) {
        // Handle window resize
        _settings.width = event.resize.width;
        _settings.height = event.resize.height;
        if (_backend) {
            _backend->onResize(event.resize.width, event.resize.height);
        }
    });
    MT_LOG_INFO("Render System Initialized");
    return true;
}

b8 mtRenderSystem::shutdown() {
    // Shutdown the rendering backend
    if (_backend) {
        MT_DELETE((mtVulkanBackend*)_backend, mtVulkanBackend);
    }
    return true;
}

void mtRenderSystem::renderFrame(const mtRenderPacket& packet) {
    // Process the render packet and issue draw calls
    // if (_backend) {
    //     _backend->renderFrame();
    // }
    //
    float speed = 2.0f;
    static float z = 3.0f;
    z += speed * packet.delta;

    if (!_backend->renderPrepare())
            return;
    _backend->renderBegin();

    glm::mat4 projection = glm::perspective(
        glm::radians(45.0f), 
        (float)_settings.width / (float)_settings.height, 
        0.1f, 
        100.0f
    );
    glm::mat4 view = glm::lookAt(
        glm::vec3(0.0f, 0.0f, z),   // eye: 相机位置
        glm::vec3(0.0f, 0.0f, 0.0f),   // center: 观察目标点
        glm::vec3(0.0f, 1.0f, 0.0f)    // up: 相机的上方向（世界坐标）
    ); 
    _backend->updateGlobalState(
        projection, 
        view, 
        glm::vec3(0.0f, 0.0f, 0.0f), 
        glm::vec4(0.0f,0.0f,0.0f,0.0f), 
        0);

    _backend->renderEnd();
    _backend->renderPresent();
}
