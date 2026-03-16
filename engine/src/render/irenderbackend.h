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

    virtual void updateObject(mtGeometryData& geometry) = 0;
};
