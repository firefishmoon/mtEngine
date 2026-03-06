#pragma once

#include "defines.h"
#include <glm/glm.hpp>

struct mtGlobalUniformObject {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 reserved0;
    glm::mat4 reserved1;
};

struct mtVertex {
    glm::vec3 position;
    // glm::vec3 normal;
    // glm::vec2 uv;
};
