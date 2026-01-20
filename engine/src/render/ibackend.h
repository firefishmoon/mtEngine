#pragma once

#include "../defines.h"

class mtIBackend {
public:
    virtual ~mtIBackend() = default;
    virtual b8 initialize() = 0;
    virtual b8 shutdown() = 0;
    virtual b8 renderFrame() = 0;
    virtual b8 onResize(u32 width, u32 height) = 0;
};
