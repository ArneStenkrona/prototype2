#include "shapes.h"

bool AABB::intersect(AABB const & a, AABB const & b) {
    return !(a.lowerBound.x > b.upperBound.x ||
             a.upperBound.x < b.lowerBound.x ||
             a.lowerBound.y > b.upperBound.y ||
             a.upperBound.y < b.lowerBound.y ||
             a.lowerBound.z > b.upperBound.z ||
             a.upperBound.z < b.lowerBound.z);
           
}

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