#include "collision.h"

#include "src/util/physics_util.h"
#include "src/util/math_util.h"

#include <glm/gtx/norm.hpp>
#include <glm/gtx/component_wise.hpp>
#include <glm/gtx/matrix_operation.hpp>
#include <glm/gtx/string_cast.hpp>

// Thank you, Turanszkij: https://wickedengine.net/2020/04/26/capsule-collision-detection/
void collideCapsuleMesh(CollisionPackage &      package,
                        CapsuleCollider const & capsule,
                        Polygon const *         polygons,
                        size_t                  nPolygons) {
    Transform & transform = *package.transform;

    for (size_t i = 0; i < nPolygons; ++i) {
        glm::vec4 a{capsule.offset, 1.0f};
        glm::vec4 b{capsule.offset + glm::vec3{0.0f, capsule.height, 0.0f}, 1.0f};

        glm::mat4 tform = glm::translate(glm::mat4(1.0f), transform.position) * glm::toMat4(glm::normalize(transform.rotation));

        glm::vec3 A = tform * a;
        glm::vec3 B = tform * b;

        glm::vec3 CapsuleNormal = glm::normalize(A - B); 
        glm::vec3 LineEndOffset = CapsuleNormal * capsule.radius; 
        // Capsule end-points
        glm::vec3 pointA = A - LineEndOffset; 

        Polygon const & p = polygons[i];
        // plane normal
        glm::vec3 n = glm::cross(p.b - p.a, p.c - p.a);
        if (glm::length(n) == 0.0f) continue;
        n = glm::normalize(n);
    
        float nDotCn = glm::dot(n, CapsuleNormal);
        glm::vec3 reference_point;
        if (glm::abs(nDotCn) > 0.0001f) {
            float t = glm::dot(n, (p.a - pointA) / glm::abs(nDotCn));
            glm::vec3 line_plane_intersection = pointA + CapsuleNormal * t;
            reference_point = physics_util::closestPointOnTriangle(p.a, p.b, p.c, line_plane_intersection);
        } else {
            reference_point = p.a;
        }
        
        // The center of the best sphere candidate:
        glm::vec3 center = physics_util::closestPointOnLineSegment(A, B, reference_point);

        // triangle-sphere intersection
        float dist = glm::dot(center - p.a, n); // signed distance between sphere and plane 
        if (dist < 0.0f) continue; // cull back-facing triangle
        if (abs(dist) > capsule.radius)  continue; // no intersection

        glm::vec3 point0 = center - n * dist; // projected sphere center on triangle plane
 
        // Now determine whether point0 is inside all triangle edges: 
        glm::vec3 c0 = glm::cross(point0 - p.a, p.b - p.a);
        glm::vec3 c1 = glm::cross(point0 - p.b, p.c - p.b);
        glm::vec3 c2 = glm::cross(point0 - p.c, p.a - p.c);
        bool inside = glm::dot(c0, n) <= 0 && glm::dot(c1, n) <= 0 && glm::dot(c2, n) <= 0;

        float radiussq = capsule.radius * capsule.radius; // sphere radius squared
        // Edge 1:
        glm::vec3 point1 = physics_util::closestPointOnLineSegment(p.a, p.b, center);
        glm::vec3 v1 = center - point1;
        float distsq1 = glm::dot(v1, v1);
        bool intersects = distsq1 < radiussq;
        
        // Edge 2:
        glm::vec3 point2 = physics_util::closestPointOnLineSegment(p.b, p.c, center);
        glm::vec3 v2 = center - point2;
        float distsq2 = glm::dot(v2, v2);
        intersects |= distsq2 < radiussq;
        
        // Edge 3:
        glm::vec3 point3 = physics_util::closestPointOnLineSegment(p.c, p.a, center);
        glm::vec3 v3 = center - point3;
        float distsq3 = glm::dot(v3, v3);
        intersects |= distsq3 < radiussq;

        if (inside || intersects) {
            glm::vec3 best_point = point0;
            glm::vec3 intersection_vec;
            
            if(inside) {
                intersection_vec = center - point0;
            } else {
                glm::vec3 d = center - point1;
                float best_distsq = glm::dot(d, d);
                best_point = point1;
                intersection_vec = d;
            
                d = center - point2;
                float distsq = glm::dot(d, d);
                if (distsq < best_distsq) {
                    distsq = best_distsq;
                    best_point = point2;
                    intersection_vec = d;
                }
            
                d = center - point3;
                distsq = glm::dot(d, d);
                if (distsq < best_distsq) {
                    distsq = best_distsq;
                    best_point = point3; 
                    intersection_vec = d;
                }
            }
            
            float len = glm::length(intersection_vec);

            CollisionResult res{};
            if (len > 0.0f && len < capsule.radius) {
                float penetration_depth = capsule.radius - len;
                glm::vec3 penetration_normal = intersection_vec / len;
                res.impulse = penetration_depth * (penetration_normal + 0.0001f);
                res.collisionNormal = penetration_normal;
                res.collisionDepth = penetration_depth;
            } else {
                res.collisionNormal = n;
            }
            res.intersectionPoint = best_point;

            handleCollision(package, res);
        }
    }
}

void collideCapsuleCapsule(CollisionPackage &      packageA,
                           CapsuleCollider const & capsuleA,
                           CollisionPackage &      packageB,
                           CapsuleCollider const & capsuleB) {
    // capsule A:
    glm::vec4 aa{capsuleA.offset, 1.0f};
    glm::vec4 ab{capsuleA.offset + glm::vec3{0.0f, capsuleA.height, 0.0f}, 1.0f};

    Transform & transformA = *packageA.transform;
    glm::mat4 tformA = glm::translate(glm::mat4(1.0f), transformA.position) * glm::toMat4(glm::normalize(transformA.rotation));

    glm::vec3 a_A = tformA * aa;
    glm::vec3 a_B = tformA * ab;

    // capsule B:
    glm::vec4 ba{capsuleB.offset, 1.0f};
    glm::vec4 bb{capsuleB.offset + glm::vec3{0.0f, capsuleB.height, 0.0f}, 1.0f};

    Transform & transformB = *packageB.transform;
    glm::mat4 tformB = glm::translate(glm::mat4(1.0f), transformB.position) * glm::toMat4(glm::normalize(transformB.rotation));

    glm::vec3 b_A = tformB * ba;
    glm::vec3 b_B = tformB * bb;

    // vectors between line endpoints:
    glm::vec3 v0 = b_A - a_A; 
    glm::vec3 v1 = b_B - a_A; 
    glm::vec3 v2 = b_A - a_B; 
    glm::vec3 v3 = b_B - a_B;
    
    // squared distances:
    float d0 = glm::dot(v0, v0); 
    float d1 = glm::dot(v1, v1); 
    float d2 = glm::dot(v2, v2); 
    float d3 = glm::dot(v3, v3);
    
    // select best potential endpoint on capsule A:
    glm::vec3 bestA;
    if (d2 < d0 || d2 < d1 || d3 < d0 || d3 < d1) {
        bestA = a_B;
    } else {
        bestA = a_A;
    }
    
    // select point on capsule B line segment nearest to best potential endpoint on A capsule:
    glm::vec3 bestB = physics_util::closestPointOnLineSegment(b_A, b_B, bestA);
    
    // now do the same for capsule A segment:
    bestA = physics_util::closestPointOnLineSegment(a_A, a_B, bestB);

    glm::vec3 penetration_normal = bestA - bestB;
    float len = glm::length(penetration_normal);
    penetration_normal /= len;  // normalize
    float penetration_depth = capsuleA.radius + capsuleB.radius - len;
    bool intersects = penetration_depth > 0;
    if (intersects) {
        CollisionResult res{};
        res.impulse = penetration_depth * (penetration_normal + 0.0001f);
        res.collisionNormal = penetration_normal;
        res.collisionDepth = penetration_depth;
        // TODO: better heuristic for finding intersection point
        res.intersectionPoint = glm::mix(bestA, bestB, 0.5f);

        handleCollision(packageA, res);
    }
}

void handleCollision(CollisionPackage & package,
                     CollisionResult const & result) {
    switch (package.type) {
        case COLLIDER_TYPE_COLLIDE: {
            package.transform->position += result.impulse;

            bool groundCollision = glm::dot(result.collisionNormal, glm::vec3{0.0f,1.0f,0.0f}) > 0.3f;

            if (groundCollision) {
                package.physics->isGrounded = true;
                // TODO: figure out a way to pick no more
                //       than one ground normal
                package.physics->groundNormal = result.collisionNormal;
            }

            break;
        }
        default: {}
    }
}