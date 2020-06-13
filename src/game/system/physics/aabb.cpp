#include "aabb.h"

bool AABB::intersect(AABB const & a, AABB const & b) {
    return !(a.lowerBound.x > b.upperBound.x ||
             a.upperBound.x < b.lowerBound.x ||
             a.lowerBound.y > b.upperBound.y ||
             a.upperBound.y < b.lowerBound.y ||
             a.lowerBound.z > b.upperBound.z ||
             a.upperBound.z < b.lowerBound.z);
}

// From "Realtime Collision Detection" by Christer Ericson
bool AABB::intersectRay(AABB const & a, 
                        glm::vec3 const & origin,
                        glm::vec3 const & direction,
                        float maxDistance) {
    float tmin = 0.0f; // set to -FLT_MAX to get first hit on line
    float tmax = maxDistance; // set to max distance ray can travel (for segment)
    // For all three slabs
    for (int i = 0; i < 3; i++) {
        if (std::abs(direction[i]) < 0.00001 /*epsilon*/) {
            // Ray is parallel to slab. No hit if origin not within slab
            if (origin[i] < a.lowerBound[i] || origin[i] > a.upperBound[i]) return false;
        } else {
            // Compute intersection t value of ray with near and far plane of slab
            float ood = 1.0f / direction[i];
            float t1 = (a.lowerBound[i] - origin[i]) * ood;
            float t2 = (a.upperBound[i] - origin[i]) * ood;
            // Make t1 be intersection with near plane, t2 with far plane
            if (t1 > t2) {
                auto temp = t1;
                t1 = t2;
                t2 = temp;
            }
            // Compute the intersection of slab intersection intervals
            if (t1 > tmin) tmin = t1;
            if (t2 < tmax) tmax = t2;
            // Exit with no collision as soon as slab intersection becomes empty 
            if (tmin > tmax) {
                return false;
            }
        }
    }
    // Ray intersects all 3 slabs. Return point (q) and intersection t value (tmin)
    //q = p + d * tmin;

    return true;
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
    upperBound = glm::max(upperBound, rhs.upperBound);
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