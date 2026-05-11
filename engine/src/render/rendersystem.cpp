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
    mtEventSystem::getInstance()->registerEvent(mtEventType::WINDOW_RESIZE, [this](mtEvent event) {
        _settings.width = event.resize.width;
        _settings.height = event.resize.height;
        if (_backend) {
            _backend->onResize(event.resize.width, event.resize.height);
        }
    });

    // Initialize texture pool
    for (int i = 0; i < MT_TEXTURE_MAX_COUNT; ++i) {
        _texturePool[i].id = mtTextureHandle::INVALID_HANDLE;
    }

    // Initialize shader pool
    for (int i = 0; i < MT_SHADER_MAX_COUNT; ++i) {
        _shaderPool[i].id = 0;
        _shaderPool[i].internalData = nullptr;
    }

    // Initialize program pool
    for (int i = 0; i < MT_SHADER_MAX_COUNT; ++i) {
        _programPool[i].id = 0;
        _programPool[i].internalData = nullptr;
    }

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
    static float z = 3.0f;

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
        glm::vec3(0.0f, 0.0f, z),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    _backend->updateGlobalState(
        projection,
        view,
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec4(0.0f,0.0f,0.0f,0.0f),
        0);

    mtRenderGeometry renderGeometry;
    renderGeometry.geometry = geometry;

    // Only look up texture if the handle is valid
    if (geometry.textureHandle.id != mtTextureHandle::INVALID_HANDLE && geometry.textureHandle.id < MT_TEXTURE_MAX_COUNT) {
        renderGeometry.texture = &_texturePool[geometry.textureHandle.id];
    } else {
        renderGeometry.texture = nullptr;
    }

    // If geometry has a valid program handle, update its non-texture uniforms before drawing
    if (geometry.programHandle.id != mtProgramHandle::INVALID_HANDLE &&
        geometry.programHandle.id != 0 &&
        geometry.programHandle.id <= MT_SHADER_MAX_COUNT) {
        // Textures are bound inside updateObject where the backend knows the internal structure
        renderGeometry.texture = renderGeometry.texture; // pass through
    }

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

mtTextureHandle mtRenderSystem::createTexture(const u8* pixels, mtTextureInfo& info) {
    mtTextureHandle handle = acquireTexture();
    if (handle.id == mtTextureHandle::INVALID_HANDLE)
        return handle;

    mtTexture& texture = _texturePool[handle.id];
    texture.width = info.width;
    texture.height = info.height;
    texture.channelCount = info.channelCount;
    texture.hasTransparency = info.hasTransparency;
    _backend->createTexture(pixels, texture);

    return handle;
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

mtShaderHandle mtRenderSystem::createShader(const u8 data, u32 dataSize) {
    // Find free slot in shader pool
    u32 index = MT_SHADER_MAX_COUNT;
    for (u32 i = 0; i < MT_SHADER_MAX_COUNT; ++i) {
        if (_shaderPool[i].id == 0) {
            index = i;
            break;
        }
    }
    if (index == MT_SHADER_MAX_COUNT) {
        MT_LOG_ERROR("Shader pool is full");
        return {mtShaderHandle::INVALID_HANDLE};
    }

    // Determine shader stage from data signature or convention
    // For now, we require the caller to have set up the stage externally
    // The shader data is raw SPIR-V bytes
    VkShaderStageFlagBits stage = VK_SHADER_STAGE_VERTEX_BIT;
    // Simple heuristic: odd-indexed shaders are fragment, even are vertex
    // In practice, this should be passed as a parameter
    (void)data;
    (void)dataSize;

    // Delegate to backend for actual creation
    mtShaderHandle handle = _backend->createShader(&data, dataSize, stage);
    if (handle.id != mtShaderHandle::INVALID_HANDLE) {
        _shaderPool[handle.id] = {handle.id, nullptr};
    }
    return handle;
}

void mtRenderSystem::destroyShader(mtShaderHandle handle) {
    if (handle.id == mtShaderHandle::INVALID_HANDLE || handle.id >= MT_SHADER_MAX_COUNT)
        return;
    if (_shaderPool[handle.id].id == 0)
        return;

    _backend->destroyShader(handle);
    _shaderPool[handle.id].id = 0;
    _shaderPool[handle.id].internalData = nullptr;
}

mtProgramHandle mtRenderSystem::createProgram(mtShaderHandle vertexShader, mtShaderHandle fragmentShader, mtProgramConfig& config) {
    // Find free slot in program pool
    u32 index = MT_SHADER_MAX_COUNT;
    for (u32 i = 0; i < MT_SHADER_MAX_COUNT; ++i) {
        if (_programPool[i].id == 0) {
            index = i;
            break;
        }
    }
    if (index == MT_SHADER_MAX_COUNT) {
        MT_LOG_ERROR("Program pool is full");
        return {mtProgramHandle::INVALID_HANDLE};
    }

    mtProgramHandle handle = _backend->createProgram(vertexShader, fragmentShader, config);
    if (handle.id != mtProgramHandle::INVALID_HANDLE) {
        _programPool[handle.id] = {handle.id, nullptr};
    }
    return handle;
}
