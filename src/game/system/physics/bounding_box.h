#ifndef BOUNDING_BOX_H
#define BOUNDING_BOX_H

#include "src/container/array.h"

#include <glm/glm.hpp>

struct BoundingBox {
    prt::array<glm::vec3, 8> vertices;
};

#endif
