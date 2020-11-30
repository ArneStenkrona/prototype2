#ifndef BILLBOARD_H
#define BILLBOARD_H

#include "glm/glm.hpp"

#include <cstdint>

#include "src/graphics/geometry/texture.h"

struct Billboard {
    glm::vec4 color;
    glm::vec2 size;
    int32_t textureIndex = -1;
};

#endif