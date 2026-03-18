#include "rendersystem.h"
#include "vulkan/vulkan_backend.h"
#include "core/loggersystem.h"
#include "core/memorysystem.h"
#include "core/eventsystem.h"
#include "render/system/texture_system.h"
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

template<> MT_API mtRenderSystem* Singleton<mtRenderSystem>::_instance = nullptr;

// mtGeometry geometry = {};
// mtTexture defaultTexture = {};
// mtTexture testTexture = {};

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

    mtTextureSystem::instance();
    // mtEventSystem::getInstance()->registerEvent(mtEventType::FRAME, [this](mtEvent event) {
    //     this->draw({event.fdata});
    //
    // });
    mtEventSystem::getInstance()->registerEvent(mtEventType::WINDOW_RESIZE, [this](mtEvent event) {
        // Handle window resize
        _settings.width = event.resize.width;
        _settings.height = event.resize.height;
        if (_backend) {
            _backend->onResize(event.resize.width, event.resize.height);
        }
    });

    // set all textures are available
    for (int i = 0; i < MT_TEXTURE_MAX_COUNT; ++i) {
        _texturePool[i].id = mtTextureHandle::INVALID_HANDLE;
    }

    // TEST CODE
    // const u32 tex_dimension = 256;
    // const u32 channels = 4;
    // const u32 pixel_count = tex_dimension * tex_dimension;
    // u8 pixels[pixel_count * channels];
    // memset(pixels, 255, sizeof(u8) * pixel_count * channels);
    // // Each pixel.
    // for (u64 row = 0; row < tex_dimension; ++row) {
    //     for (u64 col = 0; col < tex_dimension; ++col) {
    //         u64 index = (row * tex_dimension) + col;
    //         u64 index_bpp = index * channels;
    //         if (row % 2) {
    //             if (col % 2) {
    //                 pixels[index_bpp + 0] = 0;
    //                 pixels[index_bpp + 1] = 0;
    //             }
    //         } else {
    //             if (!(col % 2)) {
    //                 pixels[index_bpp + 0] = 0;
    //                 pixels[index_bpp + 1] = 0;
    //             }
    //         }
    //     }
    // }
    //
    // defaultTexture.width = tex_dimension;
    // defaultTexture.height = tex_dimension;
    // defaultTexture.hasTransparency = false;
    // defaultTexture.channelCount = 4;
    // _backend->createTexture(pixels, defaultTexture);

    // geometry.texture = defaultTexture;

    // loadTextureFromFile(_backend, "cobblestone", testTexture);
    //
    // geometry.texture = testTexture;
    //
    // mtEventSystem::getInstance()->registerEvent(mtEventType::DEBUG, [this](mtEvent event) {
    //     geometry.texture = geometry.texture.internalData == defaultTexture.internalData ? testTexture : defaultTexture;
    // });

    MT_LOG_INFO("Render System Initialized");
    return true;
}

b8 mtRenderSystem::shutdown() {
    // Shutdown the rendering backend
    if (_backend) {
        // _backend->destroyTexture(defaultTexture);
        // _backend->destroyTexture(testTexture);
        MT_DELETE((mtVulkanBackend*)_backend, mtVulkanBackend);
    }
    return true;
}

void mtRenderSystem::draw(const mtGeometry& geometry) {
    // Process the render packet and issue draw calls
    // if (_backend) {
    //     _backend->renderFrame();
    // }
    //
    // MT_LOG_INFO("Rendering frame with delta time: {}", packet.delta);

    float speed = 2.0f;
    static float z = 3.0f;
    // z += speed * packet.delta;

    // static f32 angle = 0.01f;
    // angle += 0.1f;
    // glm::quat quat = glm::angleAxis(glm::radians(angle), glm::vec3(0, 0, -1));
    // glm::mat4 model = glm::mat4_cast(quat);

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

    // packet.geometry.model = model;
    mtRenderGeometry renderGeometry;
    renderGeometry.geometry = geometry;
    renderGeometry.texture = &_texturePool[geometry.textureHandle.id];
    _backend->updateObject(renderGeometry);

    _backend->renderEnd();
    _backend->renderPresent();
}

// mtTexture mtRenderSystem::acquireTexture(const std::string& name, mtTextureInfo& outInfo, bool autoRelease) {
//     // mtTextureHandle handle = { mtTextureHandle::INVAILD_HANDLE };
//     // u32 index = _textureCreateIndex;
//     if (!loadTextureFromFile(_backend, name, _texturePool[_textureCreateIndex])) {
//         // handle.id = _textureCreateIndex;
//         throw std::runtime_error("acquireTexture failed.");
//     }
//     return _texturePool[_textureCreateIndex++];
// }


mtTextureHandle mtRenderSystem::acquireTexture() {
    mtTextureHandle handle = { mtTextureHandle::INVALID_HANDLE };
    for (int i = 0; i < MT_TEXTURE_MAX_COUNT; ++i) {
        if (_texturePool[i].id == mtTextureHandle::INVALID_HANDLE) {
            handle.id = i;
            _texturePool[i].id = i;
            break;
        }
    }
    return handle;
}

b8 mtRenderSystem::createTexture(mtTextureHandle handle, const u8* pixels, mtTextureInfo& info) {
    mtTexture& texture = _texturePool[handle.id];
    texture.width = info.width;
    texture.height = info.height;
    texture.channelCount = info.channelCount;
    texture.hasTransparency = info.hasTransparency;
    _backend->createTexture(pixels, texture);

    return true;
}

void mtRenderSystem::destroyTexture(mtTextureHandle handle) {
    mtTexture& texture = _texturePool[handle.id];
    if (texture.id == mtTextureHandle::INVALID_HANDLE)
        return;

    _backend->destroyTexture(texture);
    texture.id = mtTextureHandle::INVALID_HANDLE;
    texture.width = 0;
    texture.height = 0;
    texture.hasTransparency = 0;
    texture.channelCount = 0;
    texture.internalData = 0;
}
