#include "parametric_shapes.h"

#include "src/graphics/geometry/model.h" 

#include <cmath>

#include <iostream>
void parametric_shapes::createQuad(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Quad quad) {
    uint32_t nw = quad.resW + 2;
    uint32_t nh = quad.resH + 2; 
    vertices.resize(nw * nh);
    indices.resize(((nw - 1) * (nh - 1)) * 6);
    // vertices
    for (uint32_t iw = 0; iw < nw; iw++) {
         for (uint32_t ih = 0; ih < nh; ih++) {
             float fw = (float(iw) / float(nw-1)); 
             float fh = (float(ih) / float(nh-1)); 
             Vertex v;
             v.pos = glm::vec3{ fw * quad.width, 0.0f, fh * quad.height };
             v.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
             v.texCoord = glm::vec2{ fw, fh};
             vertices[iw + (ih * nw)] = v;
        }
    }
    uint32_t index = 0;
    // indices
    for (uint32_t iw = 0; iw < nw - 1; iw++) {
         for (uint32_t ih = 0; ih < nh - 1; ih++) {
            // triangle 1
            indices[index++] = iw + (ih * nw);
            indices[index++] = iw + 1 + (ih * nw);
            indices[index++] = iw + ((ih + 1) * nw);
            // triangle 2
            indices[index++] = iw + 1 + (ih * nw);
            indices[index++] = iw + 1 + ((ih + 1) * nw);
            indices[index++] = iw + ((ih + 1) * nw);
        }
    }
}

// void parametric_shapes::createCuboid(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cuboid cuboid) {
// /*
//     float width = 1.0f;
//     float height = 1.0f;
//     float depth = 1.0f;
//     uint32_t resW = 0;
//     uint32_t resH = 0;
//     uint32_t resD = 0;
// */
//     /* face 1 */
//     uint32_t nx = cuboid.resD;
//     uint32_t ny = cuboid.resH; 
//     // vertices
//     /*for (uint32_t ix = 0; ix < nx; ix++) {
//         for (uint32_t iy = 0; iy < ny; iy++) {
//             float fx = (float(ix) / float(nx-1)); 
//             float fy = (float(iy) / float(ny-1)); 
//             Vertex v;
//             v.pos = glm::vec3{ 0.5f * cuboid.resW, iy, ix };
//             v.normal = glm::vec3{ 1.0f, 0.0f, 0.0f };
//             //v.texCoord = glm::vec2{ fw, fh};
//             vertices[ix + (iy * nx)] = v;
//         }
//     }
// }

void parametric_shapes::createSphere(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Sphere sphere) {
    uint32_t ntheta = sphere.res + 2;
    uint32_t nphi = ntheta/2;
    vertices.resize(nphi * ntheta);
    indices.resize((ntheta * (nphi - 1)) * 6);
    uint32_t vi = 0;
    for (uint32_t iphi = 0; iphi < nphi; iphi++) {
        float phi = (float(iphi) / float(nphi-1)) * glm::pi<float>();
        for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
            float theta = (float(itheta) / float(ntheta-1)) * 2 * glm::pi<float>();
            Vertex v; 
            v.pos = glm::vec3{sphere.radius * glm::cos(theta) * glm::sin(phi),
                              sphere.radius * glm::sin(theta) * glm::sin(phi), 
                              sphere.radius * glm::cos(phi)};                
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2{theta / (2 * glm::pi<float>()), phi / glm::pi<float>()};
            vertices[vi++] = v;
        }
    }

    uint32_t index = 0;
    // indices
    // top cap
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = itheta;
        indices[index++] = ntheta + itheta;
        indices[index++] = ntheta + ((itheta + 1) % ntheta);
    }
    for (uint32_t iphi = 1; iphi < nphi - 2; iphi++) {
         for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
            // triangle 1
            indices[index++] = (iphi + 1) * ntheta + itheta;
            indices[index++] = iphi * ntheta + ((itheta + 1) % ntheta);
            indices[index++] = iphi * ntheta + itheta;
            // triangle 2
            indices[index++] = (iphi + 1) * ntheta + itheta;
            indices[index++] = (iphi + 1) * ntheta + ((itheta + 1) % ntheta);
            indices[index++] = iphi * ntheta + ((itheta + 1) % ntheta);
        }
    }
    // bottom cap
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = (nphi - 2) * ntheta + ((itheta + 1) % ntheta);
        indices[index++] = (nphi - 2) * ntheta + itheta;
        indices[index++] = (nphi - 1) * ntheta + itheta;
    }
}

/*
void parametric_shapes::createCylinder(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cylinder cylinder) {

}

void parametric_shapes::createCapsule(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Capsule capsule) {

}
*/