#pragma once

#include "defines.h"
#include <glm/glm.hpp>

struct GlobalUniformObject {
    glm::mat4 projection;
    glm::mat4 view;
    glm::mat4 reserved0;
    glm::mat4 reserved1;
};
