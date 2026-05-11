#pragma once

#include "../defines.h"
#include "render_types.h"
#include <glm/glm.hpp>
#include <string>

class mtIRenderBackend {
public:
    virtual ~mtIRenderBackend() = default;
    virtual b8 initialize(u32 width, u32 height) = 0;
    virtual b8 shutdown() = 0;
    // virtual b8 renderFrame() = 0;
    virtual b8 onResize(u32 width, u32 height) = 0;
    virtual b8 renderPrepare() = 0;
    virtual b8 renderBegin() = 0;
    virtual b8 renderEnd() = 0;
    virtual b8 renderPresent() = 0;

    virtual void createTexture(const u8* pixels, mtTexture &texture) = 0;

    virtual void destroyTexture(mtTexture& texture) = 0;

    virtual void updateGlobalState(
        glm::mat4 projection,
        glm::mat4 view,
        glm::vec3 viewPosition,
        glm::vec4 ambientColor,
        s32 mode
    ) = 0;

    virtual void updateObject(const mtRenderGeometry& geometry) = 0;

    // Shader management
    virtual mtShaderHandle createShader(const u8* data, u32 dataSize, u32 stage) = 0;
    virtual void destroyShader(mtShaderHandle handle) = 0;

    // Program management
    virtual mtProgramHandle createProgram(mtShaderHandle vertexShader, mtShaderHandle fragmentShader, mtProgramConfig& config) = 0;
    virtual void destroyProgram(mtProgramHandle handle) = 0;

    // Uniform updates
    virtual void updateUniform(mtProgramHandle program, const std::string& name, const void* data, u32 size) = 0;
};
