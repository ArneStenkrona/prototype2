#ifndef SHAPES_H
#define SHAPES_H

#include <glm/glm.hpp>

struct AABB {
    glm::vec3 lowerBound;
    glm::vec3 upperBound;

    /**
     * @return surface area of the AABB
     */
    float area() const;
    /**
     * @return volume of the AABB 
     */
    float volume() const;

    /**
     * expands the AABB to the AABB enclosing
     * both operand AABB's
     * @param rhs right-hand side of the operation 
     * @return reference to this
     */
    AABB& operator+=(AABB const & rhs);
    /**
     * returns the AABB enclosing both operands'
     * AABB's
     * @param lhs left-hand side of the operation 
     * @param rhs right-hand side of the operation 
     * @return AABB that encloses both lhs and rhs
     */
    friend AABB operator+(AABB lhs, AABB const & rhs) {
        lhs += rhs;
        return lhs;
    }
};

#endif