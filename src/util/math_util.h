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
    
    glm::mat4 safeLookAt(glm::vec3 const & lookFrom,
                         glm::vec3 const & lookTo,
                         glm::vec3 const & up,
                         glm::vec3 const & alternativeUp);

    /**
     * Calculate linear interpolation
     * @param a to interpolate from
     * @param b to interpolate to
     * @param t interpolation factor
     * @return interpolated value
     */
    inline float lerp(float a, float b, float t) { return (1.0f - t) * a + t * b; }

    /**
     * returns a diagonalizing matrix Q
     * such that D = Q * A * Transpose(Q) is
     * a diagonal matrix
     * @param matrix A
     * @return diagonalizing matrix
     */
    glm::mat3 diagonalizer(glm::mat3 const & A);

};

#endif