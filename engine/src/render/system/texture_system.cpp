#include "render/system/texture_system.h"
#include "render/rendersystem.h"
#include "core/loggersystem.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

template<> MT_API mtTextureSystem* Singleton<mtTextureSystem>::_instance = nullptr;

b8 mtTextureSystem::initialize() {
    return true;
}

b8 mtTextureSystem::shutdown() {
    return true;
}


b8 mtTextureSystem::loadTextureFromFile(mtTextureHandle& handle, const std::string& name, mtTextureInfo& outInfo) {
    stbi_set_flip_vertically_on_load(true);
    std::string fileName = std::format("assets/textures/{}.png", name);
    u8* data = stbi_load(fileName.c_str(), (int*)&outInfo.width, (int*)&outInfo.height, (int*)&outInfo.channelCount, 4);
    if (!data) {
        MT_LOG_ERROR("Failed to load texture from file: {}", fileName);
        return false;
    }

    MT_LOG_INFO("Loaded texture '{}' with dimensions: {}x{} and channels: {}", name, outInfo.width, outInfo.height, outInfo.channelCount);
    outInfo.channelCount = 4;

    //backend->createTexture(data, texture);
    handle = mtRenderSystem::getInstance()->createTexture(data, outInfo);

    stbi_image_free(data);
    return true;
}

mtTextureHandle mtTextureSystem::acquireTexture(const std::string& name, mtTextureInfo& outInfo, bool autoRelease) {
    if (_textureMap.contains(name)) {
        _textureMap[name].references += 1;
        return _textureMap[name].handle;
    }
    mtTextureHandle handle = { mtTextureHandle::INVALID_HANDLE };
    loadTextureFromFile(handle, name, outInfo);
    if (handle.id == mtTextureHandle::INVALID_HANDLE) {
        MT_LOG_ERROR("mtRenderSystem::acquireTexture failed.");
        return handle;
    }
    _textureMap[name] = {
        handle,
        autoRelease,
        1
    };
    MT_LOG_DEBUG("createTexture {}", name.c_str());
    return handle;
}

void mtTextureSystem::releaseTexture(const std::string& name) {
    if (!_textureMap.contains(name))
        return;

    _textureMap[name].references -= 1;
    if (_textureMap[name].autoRelease && _textureMap[name].references == 0) {
        mtRenderSystem::getInstance()->destroyTexture(_textureMap[name].handle);
        _textureMap.erase(name);
        MT_LOG_DEBUG("destroyTexture {}", name.c_str());
    }
}
