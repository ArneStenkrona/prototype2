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
    for (size_t i = 0; i < nPolygons; ++i) {
        glm::vec3 A = package.transform->position + capsule.offset;
        glm::vec3 B = package.transform->position + capsule.offset + glm::vec3{0.0f, capsule.height, 0.0f};

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
    glm::vec3 a_A = packageA.transform->position + capsuleA.offset;
    glm::vec3 a_B = packageA.transform->position + capsuleA.offset + glm::vec3{0.0f, capsuleA.height, 0.0f};
    
    // capsule B:
    glm::vec3 b_A = packageB.transform->position + capsuleB.offset;
    glm::vec3 b_B = packageB.transform->position + capsuleB.offset + glm::vec3{0.0f, capsuleB.height, 0.0f};
    
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

bool collideSphereMesh(glm::vec3 const & sourcePoint, 
                       glm::vec3 const & relativeVelocity, 
                       Polygon   const * polygons,
                       size_t            nPolygons,
                       glm::vec3 &       intersectionPoint,
                       float     &       intersectionTime,
                       glm::vec3 &       collisionNormal) {
    // collision normal
    glm::vec3 cn{1.0f};
    
    intersectionTime = std::numeric_limits<float>::max();
    for (size_t i = 0; i < nPolygons; ++i) {
        Polygon const & p = polygons[i];
        // plane normal
        glm::vec3 n = glm::cross((p.b-p.a), (p.c-p.a));
        if (glm::length(n) == 0.0f) continue;
        n = glm::normalize(n);

        // skip triangle if relative velocity is not towards the triangle
        if (glm::length(relativeVelocity) > 0.0f && 
            glm::dot(glm::normalize(relativeVelocity), n) > 0.0f) {
            continue;
        }
        // plane constant
        float c = -glm::dot(n, p.a);

        float t0;
        float t1;
        float ndotv = glm::dot(n, relativeVelocity);
        float sigDist = glm::dot(sourcePoint, n) + c;
        bool embeddedInPlane = false;
        // check distance to plane and set t0,t1
        if (ndotv == 0.0f) {
            if (std::abs(sigDist) > 1.0f) {
                continue;
            } else {
                // sphere is embedded in plane
                t0 = 0.0f;
                t1 = 1.0f;
                embeddedInPlane = true;
            }
        } else {
            t0 = (1.0f - sigDist) / ndotv;
            t1 = (-1.0f - sigDist) / ndotv;

            if (t0 > t1) {
                float temp = t0;
                t0 = t1;
                t1 = temp;
            }
            if ( t1 < 0.0f || t0 > 1.0f) {
                // collision occurs outside velocity range
                continue;
            }
            t0 = glm::clamp(t0, 0.0f, 1.0f);
            t1 = glm::clamp(t1, 0.0f, 1.0f);
        }
        
        if (!embeddedInPlane) {
            // calculate plane intersection point;
            glm::vec3 pip = sourcePoint - n + t0 * relativeVelocity;
            // check if we are colliding inside the triangle
            if (physics_util::checkPointInTriangle(pip, p.a, p.b, p.c)) {
                float t = t0;
                if (t < intersectionTime) {
                    intersectionTime = t;
                    intersectionPoint = pip;
                    
                    glm::vec3 futurePos = sourcePoint + (intersectionTime * relativeVelocity);
                    cn = glm::normalize(futurePos - intersectionPoint);
                }
                // collision inside triangle
                continue;
            }
        }
        
        // sweep against vertices and edges
        // vertex;
        float av = glm::length2(relativeVelocity);
        if (av > 0.0f) {
            for (size_t j = 0; j < 3; j++) {
                float bv = 2.0f * glm::dot(relativeVelocity, sourcePoint - p[j]);
                float cv = glm::length2(p[j] - sourcePoint) - 1.0f;

                float sqrterm = (bv * bv) - (4.0f * av * cv);
                if (sqrterm >= 0.0f) {
                    float x1 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                    float x2 = (-bv + std::sqrt(sqrterm)) / (2.0f * av);
                    float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                    if (t >= 0.0f && t <= 1.0f && t < intersectionTime) {
                        intersectionTime = t;
                        intersectionPoint = p[j];

                        glm::vec3 futurePos = sourcePoint + (intersectionTime * relativeVelocity);
                        cn = glm::normalize(futurePos - intersectionPoint);
                    }
                }
            }
        
            // edges
            glm::vec3 edges[3] = { p.b - p.a, p.c - p.b, p.a - p.c };
            glm::vec3 btv[3] = { p.a - sourcePoint, p.b - sourcePoint, p.c - sourcePoint }; // base to vertex
            for (size_t j = 0; j < 3; ++j) {
                float edge2 = glm::length2(edges[j]);

                float ae = edge2 * (-glm::length2(relativeVelocity)) +
                        glm::pow(glm::dot(edges[j], relativeVelocity), 2.0f);
                float be = edge2 * 2.0f * glm::dot(relativeVelocity, btv[j]) -
                        2.0f * (glm::dot(edges[j], relativeVelocity) * glm::dot(edges[j], btv[j]));
                float ce = edge2 * (1.0f - glm::length2(btv[j])) + 
                        glm::pow(glm::dot(edges[j], btv[j]), 2.0f);

                float sqrterm = (be * be) - (4.0f * ae * ce);
                if (ae != 0.0f && sqrterm >= 0.0f) {
                    float x1 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float x2 = (-be + std::sqrt(sqrterm)) / (2.0f * ae);
                    float t = std::abs(x1) < std::abs(x2) ? x1 : x2;
                    if (t >= 0.0f && t <= 1.0f) {
                        float f0 = (glm::dot(edges[j], relativeVelocity) * t - glm::dot(edges[j], btv[j])) /
                                    glm::length2(edges[j]);
                        if (f0 >= 0.0f && f0 <= 1.0f && t < intersectionTime) {
                            intersectionTime = t;
                            intersectionPoint = p[j] + f0 * edges[j];

                            glm::vec3 futurePos = sourcePoint + (intersectionTime * relativeVelocity);
                            cn = glm::normalize(futurePos - intersectionPoint);
                        }
                    }
                }
            }
        }
    }
    collisionNormal = cn;
    return intersectionTime <= 1.0f;
}

bool collideEllipsoidEllipsoid(glm::vec3 const & ellipsoid0,
                               glm::vec3 const & sourcePoint0, 
                               glm::vec3 const & velocity0, 
                               glm::vec3 const & ellipsoid1, 
                               glm::vec3 const & sourcePoint1, 
                               glm::vec3 const & velocity1,
                               float & intersectionTime, 
                               glm::vec3 & intersectionPoint,
                               glm::vec3 & collisionNormal) {
    // Get the parameters of ellipsoid0. 
    glm::vec3 K0 = sourcePoint0; 
    glm::mat3 R0(1.0f); // axis aligned => axis is identity matrix
    // glm::mat3 D0 = glm::diagonal3x3(glm::vec3(1.0f / (ellipsoid0[0] * ellipsoid0[0]), 
    //                                           1.0f / (ellipsoid0[1] * ellipsoid0[1]), 
    //                                           1.0f / (ellipsoid0[2] * ellipsoid0[2])));

    // Get the parameters of ellipsoid1. 
    glm::vec3 K1 = sourcePoint1; 
    glm::mat3 R1(1.0f); // axis aligned => axis is identity matrix
    glm::mat3 D1 = glm::diagonal3x3(1.0f / (ellipsoid1 * ellipsoid1));

    // Compute K2.
    glm::mat3 D0NegHalf = glm::diagonal3x3(ellipsoid0);

    glm::mat3 D0Half = glm::diagonal3x3(1.0f / ellipsoid0);

    glm::vec3 K2 = (K1 - K0) * glm::transpose(R0) * D0Half;
    // Compute M2.
    glm::mat3 R1TR0D0NegHalf = R0 * D0NegHalf * glm::transpose(R1); 
    glm::mat3 M2 = D1 * R1TR0D0NegHalf * glm::transpose(R1TR0D0NegHalf);
    // Factor M2 = R*D*R^T. 
    glm::mat3 Q = math_util::diagonalizer(M2);
    glm::mat3 D = glm::transpose(Q) * M2 * Q;
    glm::mat3 R = glm::transpose(Q);
    // Compute K. 
    glm::vec3 K = K2 * glm::transpose(R);
    // Compute W.
    glm::vec3 W = D0Half * (velocity1 - velocity0) * R0 * R;
    // relative velocity is stationary
    if (glm::length(W) == 0.0f) return false;
    // Transformed ellipsoid0 is Z^T*Z = 1 and transformed ellipsoid1 is 
    // (Z-K)^T*D*(Z-K) = 0.

    // Compute the initial closest point. 
    glm::vec3 P0;
    if (computeClosestPointEllipsoids(D, K, P0) >= 0.0f) {
        // The ellipsoid contains the origin, so the ellipsoids were not 
        // separated.
        return false;
    }
    
    // float dist0 = glm::dot(P0, P0) - 1.0f;
    float dist0 = glm::dot(P0, P0) / glm::length(W) - 1.0f; // tolerance for some overlap
    if (dist0 < 0.0f) {
        // The ellipsoids are not separated.
        return false;
    }
    glm::vec3 zContact;
    if (!computeContactEllipsoids(D, K, W, intersectionTime, zContact)) {
        return false; 
    }
    // Transform contactPoint back to original space
    intersectionPoint = K0 + R0 * D0NegHalf * R * zContact;
    // collision normal is the gradient 2D(P - K)
    collisionNormal = glm::normalize(D1 * (intersectionPoint - K1));
    return -1.0f <= intersectionTime && intersectionTime <= 1.0f;
}

// thank you David Eberly: https://www.geometrictools.com/Documentation/IntersectionSweptEllipsesEllipsoids.pdf
bool computeContactEllipsoids(glm::mat3 const & D, 
                              glm::vec3 const & K, 
                              glm::vec3 const & W, 
                              float & intersectionTime, 
                              glm::vec3 & zContact) {
    float d0 = D[0][0], d1 = D[1][1], d2 = D[2][2];
    // float k0 = K[0], k1 = K[1], k2 = K[2]; 
    float w0 = W[0], w1 = W[1], w2 = W[2];

    static constexpr int maxIterations = 128; 
    static constexpr float epsilon = 1e-08f;
    intersectionTime = 0.0;
    for (int i = 0; i < maxIterations; ++i) {
        D[2][2];
        // Compute h(t).
        glm::vec3 Kmove = K + intersectionTime * W;
        float s = computeClosestPointEllipsoids(D, Kmove, zContact); 
        float tmp0 = d0 * Kmove[0] * s / (d0 * s - 1.0f);
        float tmp1 = d1 * Kmove[1] * s / (d1 * s - 1.0f);
        float tmp2 = d2 * Kmove[2] * s / (d2 * s - 1.0f);
        float h = tmp0 * tmp0 + tmp1 * tmp1 + tmp2 * tmp2 - 1.0f;
        if (fabs(h) < epsilon) {
            // We have found a root.
            return true; 
        }
        // Compute h’(t).
        float hder = 4.0f * tmp0 * w0 + 4.0f * tmp1 * w1 + 4.0f * tmp2 * w2; 
        // if (hder > 0.0f) { // this would result in division by zero
        if (hder >= 0.0f) {
            // The ellipsoid cannot intersect the sphere.
            return false; 
        }
        // Compute the next iterate tNext = t - h(t)/h’(t).
        intersectionTime -= h/hder; 
    }
    computeClosestPointEllipsoids(D, K + intersectionTime * W, zContact);
    return true; 
}

// thank you David Eberly: https://www.geometrictools.com/Documentation/IntersectionSweptEllipsesEllipsoids.pdf
float computeClosestPointEllipsoids(glm::mat3 const & D, 
                                    glm::vec3 const & K, 
                                    glm::vec3 & closestPoint) {
    float d0 = D[0][0], d1 = D[1][1], d2 = D[2][2];
    float k0 = K[0], k1 = K[1], k2 = K[2];
    float d0k0 = d0 * k0; 
    float d1k1 = d1 * k1; 
    float d2k2 = d2 * k2; 
    float d0k0k0 = d0k0 * k0; 
    float d1k1k1 = d1k1 * k1; 
    float d2k2k2 = d2k2 * k2;

    if (d0k0k0 + d1k1k1 + d2k2k2 - 1.0f < 0.0f) {
        // The ellipsoid contains the origin, so the ellipsoid and sphere are 
        // overlapping.
        return std::numeric_limits<float>::max();
    }

    static constexpr int maxIterations = 128; 
    static constexpr float epsilon = 1e-08f;

    float s = 0.0; 
    int i;
    for (i = 0; i < maxIterations; ++i) {
        // Compute f(s).
        float tmp0 = d0 * s - 1.0f;
        float tmp1 = d1 * s - 1.0f;
        float tmp2 = d2 * s - 1.0f;
        float tmp0sqr = tmp0 * tmp0;
        float tmp1sqr = tmp1 * tmp1;
        float tmp2sqr = tmp2 * tmp2;
        float f = d0k0k0 / tmp0sqr + d1k1k1 / tmp1sqr + d2k2k2 / tmp2sqr - 1.0f;
        if (fabs(f) < epsilon) {
            // We have found a root.
            break; 
        }
        // Compute f’(s).
        float tmp0cub = tmp0 * tmp0sqr;
        float tmp1cub = tmp1 * tmp1sqr;
        float tmp2cub = tmp2 * tmp2sqr;
        float fder = -2.0f*(d0 * d0k0k0/tmp0cub + d1 * d1k1k1 / tmp1cub
                            + d2 * d2k2k2 / tmp2cub);
        // Compute the next iterate sNext = s - f(s)/f’(s).
        if (fder == 0.0f) return std::numeric_limits<float>::max();;
        s -= f/fder;
    }
    closestPoint[0] = d0k0 * s / ( d0 * s - 1.0f); 
    closestPoint[1] = d1k1 * s / ( d1 * s - 1.0f); 
    closestPoint[2] = d2k2 * s / ( d2 * s - 1.0f); 
    return s;
}

void handleCollision(CollisionPackage & package,
                     CollisionResult const & result) {
    switch (package.type) {
        case COLLISION_TYPE_RESPOND: {
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