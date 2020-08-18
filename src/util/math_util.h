#ifndef PRT_MATH_UTIL_H
#define PRT_MATH_UTIL_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/quaternion.hpp>

namespace math_util {
    /**
     * Calculate look direction
     * @param lookFrom from position
     * @param lookTo to position
     * @param up up direction
     * @param alternativeUp alternative up direction
     *                      in case of singularity
     * @return look direction as normalized vector 
     */
    glm::quat safeQuatLookAt(glm::vec3 const & lookFrom,
                             glm::vec3 const & lookTo,
                             glm::vec3 const & up,
                             glm::vec3 const & alternativeUp);
};

#endif