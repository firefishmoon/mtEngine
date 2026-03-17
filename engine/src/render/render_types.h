#pragma once

#include "defines.h"
#include <glm/glm.hpp>

struct mtGlobalUniformObject {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 reserved0;
    glm::mat4 reserved1;
};

struct mtObjectUniformObject {
    glm::vec4 diffuseColor;
    glm::vec4 reserved0;
    glm::vec4 reserved1;
    glm::vec4 reserved2;
};

struct mtVertex {
    glm::vec3 position;
    // glm::vec3 normal;
    glm::vec2 uv;
};

// using mtTextureHandle = u32;

struct mtTexture {
    u32 id;
    s32 width;
    s32 height;
    s32 channelCount;
    b8 hasTransparency;
    void* internalData; // For backend-specific texture handle (e.g., Vulkan image view)
};


struct mtTextureHandle {
    u32 id;
    enum {
        INVAILD_HANDLE = 0xFFFF
    };
};

struct mtTextureInfo {
    s32 width;
    s32 height;
    s32 channelCount;
    b8 hasTransparency;
};

struct mtGeometry {
    u32 objectId;
    glm::mat4 model;
    mtTexture texture;
    // mtTextureHandle textureHandle;
};

// struct mtTexture {
//     u32 handle;
//     u32 width;
//     u32 height;
//     u8 channels;
//     b8 hasTransparency;
// };
