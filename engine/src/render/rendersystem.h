#pragma once

#include "../defines.h"
#include "../common/singleton.h"
#include "irenderbackend.h"

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

    b8 initialize() override;
    b8 shutdown() override;
    void renderFrame(const mtRenderPacket& packet);
private:
    mtRenderSettings _settings;
    mtIRenderBackend* _backend = nullptr;
};
