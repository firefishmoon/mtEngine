#pragma once

#include "../defines.h"
#include "../common/singleton.h"
#include "render_types.h"
#include "irenderbackend.h"

#include <memory>

enum class mtBackendAPI {
    OPENGL = 0,
    VULKAN,
    DIRECTX12,
    METAL,
    COUNT
};

struct mtRenderPacket {
    // Placeholder for render packet data
    // e.g., mesh data, texture references, shader info, etc.
    float delta; // delta time
    mtGeometry geometry;
};

struct mtRenderSettings {
    mtBackendAPI api;
    u32 width;
    u32 height;
};

class MT_API mtRenderSystem : public Singleton<mtRenderSystem> {
public:
    mtRenderSystem(const mtRenderSettings& settings = mtRenderSettings())
        : _settings(settings) {};

    ~mtRenderSystem() = default;

    // mtTexture acquireTexture(const std::string& name, mtTextureInfo& outInfo, bool autoRelease);

    // mtTextureHandle acquireTexture();
    mtTextureHandle createTexture(const u8* pixels, mtTextureInfo& info);
    void destroyTexture(mtTextureHandle handle);

    mtShaderHandle createShader(const u8 data, u32 dataSize);
    void destroyShader(mtShaderHandle handle);

    mtProgramHandle createProgram(mtShaderHandle vertexShader, mtShaderHandle fragmentShader, mtProgramConfig& config);

    b8 initialize() override;
    b8 shutdown() override;
    void draw(const mtGeometry& geometry);
private:
    mtTextureHandle acquireTexture();

    mtRenderSettings _settings;
    mtIRenderBackend* _backend = nullptr;

    mtTexture _texturePool[MT_TEXTURE_MAX_COUNT] = {};
    u32 _textureCreateIndex = 0;

    mtShader _shaderPool[MT_SHADER_MAX_COUNT] = {};
};
