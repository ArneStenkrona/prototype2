#include "shapes.h"

bool AABB::contains(AABB const & other) {
    return lowerBound.x <= other.lowerBound.x &&
           lowerBound.y <= other.lowerBound.y &&
           lowerBound.z <= other.lowerBound.z &&
           upperBound.x >= other.upperBound.x &&
           upperBound.y >= other.upperBound.y &&
           upperBound.z >= other.upperBound.z;
}

AABB& AABB::operator+=(AABB const & rhs) {
    lowerBound = glm::min(lowerBound, rhs.lowerBound);
    upperBound = glm::min(upperBound, rhs.upperBound);
    return *this;
}

float AABB::area() const {
    glm::vec3 d = upperBound - lowerBound;
    return 2.0f * (d.x * d.y + d.y * d.z + d.z * d.x);
}
float AABB::volume() const {
    glm::vec3 d = upperBound - lowerBound;
    return d.x * d.y * d.z;
}