#include "math_util.h"

glm::quat math_util::safeQuatLookAt(glm::vec3 const & lookFrom,
                         glm::vec3 const & lookTo,
                         glm::vec3 const & up,
                         glm::vec3 const & alternativeUp) {
    glm::vec3  direction       = lookTo - lookFrom;
    float      directionLength = glm::length(direction);

    // Check if the direction is valid; Also deals with NaN
    if(!(directionLength > 0.0001))
        return glm::quat(1, 0, 0, 0); // Just return identity

    // Normalize direction
    direction /= directionLength;

    // Is the normal up (nearly) parallel to direction?
    if(glm::abs(glm::dot(direction, up)) > .9999f) {
        // Use alternative up
        return glm::quatLookAt(direction, alternativeUp);
    } else {
        return glm::quatLookAt(direction, up);
    }
}