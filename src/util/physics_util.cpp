#include "physics_util.h"
#include <glm/gtx/norm.hpp>

// From "Realtime Collision Detection" by Christer Ericson
glm::vec3 physics_util::closestPointOnTriangle(glm::vec3 const & a,
                                               glm::vec3 const & b,
                                               glm::vec3 const & c,
                                               glm::vec3 const & p) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 bc = c - b;
    // Compute parametric position s for projection P’ of P on AB, // P’ = A + s*AB, s = snom/(snom+sdenom)
    float snom = glm::dot(p - a, ab), sdenom = glm::dot(p - b, a - b);
    // Compute parametric position t for projection P’ of P on AC, // P’ = A + t*AC, s = tnom/(tnom+tdenom)
    float tnom = glm::dot(p - a, ac), tdenom = glm::dot(p - c, a - c);
    if (snom <= 0.0f && tnom <= 0.0f) return a; // Vertex region early out
    // Compute parametric position u for projection P’ of P on BC, // P’ = B + u*BC, u = unom/(unom+udenom)
    float unom = glm::dot(p - b, bc), udenom = glm::dot(p - c, b - c);
    if (sdenom <= 0.0f && unom <= 0.0f) return b; // Vertex region early out 
    if (tdenom <= 0.0f && udenom <= 0.0f) return c; // Vertex region early out

    // P is outside (or on) AB if the triple scalar product [N PA PB] <= 0
    glm::vec3 n = glm::cross(b - a, c - a);
    float vc = glm::dot(n, glm::cross(a - p, b - p));

    // If P outside AB and within feature region of AB, // return projection of P onto AB
    if (vc <= 0.0f && snom >= 0.0f && sdenom >= 0.0f)
        return a + snom / (snom + sdenom) * ab;

    // P is outside (or on) BC if the triple scalar product [N PB PC] <= 0
    float va = glm::dot(n, glm::cross(b - p, c - p));

    // If P outside BC and within feature region of BC, // return projection of P onto BC
    if (va <= 0.0f && unom >= 0.0f && udenom >= 0.0f) {
        return b + unom / (unom + udenom) * bc;
    }

    // P is outside (or on) CA if the triple scalar product [N PC PA] <= 0
    float vb = glm::dot(n, glm::cross(c - p, a - p));

    // If P outside CA and within feature region of CA, // return projection of P onto CA
    if (vb <= 0.0f && tnom >= 0.0f && tdenom >= 0.0f)
    return a + tnom / (tnom + tdenom) * ac;
    // P must project inside face region. Compute Q using barycentric coordinates
    float u = va / (va + vb + vc);
    float v = vb / (va + vb + vc); 
    float w = 1.0f - u - v; // =vc/(va+vb+vc) 
    return u * a + v * b + w * c;
}

bool physics_util::checkPointInTriangle(glm::vec3 const & point,
                                        glm::vec3 const & pa, 
                                        glm::vec3 const & pb, 
                                        glm::vec3 const & pc) {
    // Gene's solution: https://stackoverflow.com/a/25516767 
    // glm::vec3 ba = pb - pa;
    // glm::vec3 cb = pc - pb;
    // glm::vec3 ac = pa - pc;
    // glm::vec3 n = glm::cross(ac, ba);

    // glm::vec3 px = point - pa;
    // glm::vec3 nx = glm::cross(ba, px);
    // if (glm::dot(nx, n) < 0.0f) return false;

    // px = point - pb;
    // nx = glm::cross(cb, px);
    // if (glm::dot(nx, n) < 0.0f) return false;

    // px = point - pc;
    // nx = glm::cross(ac, px);
    // if (glm::dot(nx, n) < 0.0f) return false;

    // return true;

    // Craig's solution: https://gamedev.stackexchange.com/a/152476
    // u=P2−P1
    glm::vec3 u = pb - pa;
    // v=P3−P1
    glm::vec3 v = pc - pa;
    // n=u×v
    glm::vec3 n = glm::cross(u, v);
    // w=P−P1
    glm::vec3 w = point - pa;
    // Barycentric coordinates of the projection P′of P onto T:
    // γ=[(u×w)⋅n]/n²
    float gamma = glm::dot(glm::cross(u, w), n) / glm::dot(n, n);
    // β=[(w×v)⋅n]/n²
    float beta = glm::dot(glm::cross(w, v), n) / glm::dot(n, n);
    float alpha = 1 - gamma - beta;
    // The point P′ lies inside T if:
    return ((0 <= alpha) && (alpha <= 1) &&
            (0 <= beta)  && (beta  <= 1) &&
            (0 <= gamma) && (gamma <= 1));
}

// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
glm::vec3 physics_util::closestPointOnTrianglePerimeter(glm::vec3 const & a,
                                                        glm::vec3 const & b,
                                                        glm::vec3 const & c,
                                                        glm::vec3 const & p) {
    glm::vec3 rab = closestPointOnLineSegment(a, b, p); 
    glm::vec3 rbc = closestPointOnLineSegment(b, c, p); 
    glm::vec3 rca = closestPointOnLineSegment(c, a, p);
    float distAB2 = glm::distance2(rab, p);
    float distBC2 = glm::distance2(rbc, p);
    float distCA2 = glm::distance2(rca, p);

    float minDist = distAB2;
    glm::vec3 minPoint = rab;
    if (distBC2 < minDist) {
        minDist = distBC2;
        minPoint = rbc;
    }
    if (distCA2 < minDist) {
        // minDist = distCA2;
        minPoint = rca;
    }
    return minPoint;
}


// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
glm::vec3 physics_util::closestPointOnLineSegment(glm::vec3 const & a,
                                                  glm::vec3 const & b,
                                                  glm::vec3 const & p) {
    // Determine t (the length of the vector from ‘a’ to ‘p’)
    glm::vec3 c = p - a;
    glm::vec3 v = glm::normalize(b - a); 
    float d = glm::distance(a, b);
    float t = glm::dot(v, c);
    // Check to see if ‘t’ is beyond the extents of the line segment
    if (t < 0) return a; 
    if (t > d) return b;
    // Return the point between ‘a’ and ‘b’
    v = v * t; 
    return a + v;
}

// from Generic Collision Detection for Games Using Ellipsoids
// by Paul Nettle
bool physics_util::intersectRayPlane(glm::vec3 const & planeOrigin, 
                                     glm::vec3 const & planeNormal,
                                     glm::vec3 const & rayOrigin,
                                     glm::vec3 const & rayVector,
                                     float & t) {
    float d = -glm::dot(planeNormal, planeOrigin); 
    float numer = glm::dot(planeNormal, rayOrigin) + d; 
    float denom = glm::dot(planeNormal, rayVector); 
    if (denom == 0.0f) return false;
    t = -(numer / denom);
    return true;
}

// Intersects ray r = p + td, |d| = 1, with sphere s and, if intersecting, 
// returns t value of intersection
// From "Realtime Collision Detection" by Christer Ericson
bool physics_util::intersectRaySphere(glm::vec3 const & rayOrigin, 
                                      glm::vec3 const & rayDirection, 
                                      glm::vec3 const & sphereOrigin, 
                                      float sphereRadius,
                                      float & t) {
    glm::vec3 m = rayOrigin - sphereOrigin;
    float b = glm::dot(m, rayDirection);
    float c = glm::dot(m, m) - (sphereRadius * sphereRadius);
    // Exit if r’s origin outside s (c > 0) and r pointing away from s (b > 0)
    if (c > 0.0f && b > 0.0f) return false;
    float discr = b*b - c;
    // A negative discriminant corresponds to ray missing sphere
    if (discr < 0.0f) return false;
    // Ray now found to intersect sphere, compute smallest t value of intersection 
    t = -b - glm::sqrt(discr);
    // If t is negative, ray started inside sphere so clamp t to zero
    if (t < 0.0f) t = 0.0f;
    //q = rayOrigin + t * rayDirection;
    return true;
}

// From "Realtime Collision Detection" by Christer Ericson
bool physics_util::intersectLineSegmentTriangle(glm::vec3 const & origin, 
                                                glm::vec3 const & end, 
                                                glm::vec3 const & a,
                                                glm::vec3 const & b,
                                                glm::vec3 const & c,
                                                float &t) {
    glm::vec3 ab = b - a;
    glm::vec3 ac = c - a;
    glm::vec3 qp = origin - end;
    // Compute triangle normal. Can be precalculated or cached if // intersecting multiple segments against the same triangle 
    glm::vec3 n = glm::cross(ab, ac);
    // Compute denominator d. If d <= 0, segment is parallel to or points // away from triangle, so exit early
    float d = glm::dot(qp, n);
    if (d <= 0.0f) return false;
        // Compute intersection t value of pq with plane of triangle. A ray
        // intersects iff 0 <= t. Segment intersects iff 0 <= t <= 1. Delay
        // dividing by d until intersection has been found to pierce triangle

    glm::vec3 ap = origin - a;
    t = glm::dot(ap, n);
    if (t < 0.0f) return false;
    if (t > d) return false; // For segment; exclude this code line for a ray test
    // Compute barycentric coordinate components and test if within bounds
    glm::vec3 e = glm::cross(qp, ap);
    float v = glm::dot(ac, e);
    if (v < 0.0f || v > d) return false;
    float w = -glm::dot(ab, e);
    if (w < 0.0f || v + w > d) return false;
    // Segment/ray intersects triangle. Perform delayed division and // compute the last barycentric coordinate component
    float ood = 1.0f / d;
    t *= ood;
    // v *= ood;
    // w *= ood;
    //float u = 1.0f - v - w;
    return true;
}
