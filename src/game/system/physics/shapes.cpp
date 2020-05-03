#include "shapes.h"

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