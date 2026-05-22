#pragma once

#include "defines.h"
#include <glm/glm.hpp>
#include <string>

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
#define MT_TEXTURE_MAX_COUNT 128
#define MT_SHADER_MAX_COUNT 128

struct mtTexture {
    u32 id;
    s32 width;
    s32 height;
    s32 channelCount;
    b8 hasTransparency;
    void *internalData; // For backend-specific texture handle (e.g., Vulkan image view)
};

struct mtTextureHandle {
    u32 id;
    enum { INVALID_HANDLE = 0xFFFF };
};

struct mtTextureInfo {
    s32 width;
    s32 height;
    s32 channelCount;
    b8 hasTransparency;
};

//
// shaders
//
enum class mtShaderType { VERTEX, FRAGMENT };

struct mtShaderHandle {
    u32 id;
    enum { INVALID_HANDLE = 0xFFFF };
};

struct mtShader {
    u32 id;
    void *internalData;
};

//
// programs
//
struct mtProgramHandle {
    u32 id;
    enum { INVALID_HANDLE = 0xFFFF };
};

struct mtProgram {
    u32 id;
    void *internalData;
};

enum class mtVertexAttributeType { F32, F32_2, F32_3, F32_4, MAT3, MAT4, S8, U8, S16, U16, S32, U32 };

enum class mtUniformType { F32, VEC2, VEC3, VEC4, S8, U8, S16, U16, S32, U32, MAT3, MAT4, SAMPER1D, SAMPER2D };

enum class mtUniformScope { GLOBAL, INSTANCE, LOCAL };

struct mtUniform {
    std::string name;
    mtUniformType type;
    mtUniformScope scope;
    u32 location;
};

struct mtProgramConfig {
    std::vector<mtUniform> uniforms;
};

struct mtGeometry {
    u32 objectId;
    glm::mat4 model;
    mtTextureHandle textureHandle;
    mtProgramHandle programHandle;
};

struct mtRenderGeometry {
    mtGeometry geometry;
    mtTexture *texture;
    mtProgram *program;
};

// struct mtTexture {
//     u32 handle;
//     u32 width;
//     u32 height;
//     u8 channels;
//     b8 hasTransparency;
// };
