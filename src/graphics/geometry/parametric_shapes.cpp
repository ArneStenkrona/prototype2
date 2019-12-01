#include "parametric_shapes.h"

#include "src/graphics/geometry/model.h" 

void parametric_shapes::createQuad(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Quad quad) {
    uint32_t nw = quad.resW + 2;
    uint32_t nh = quad.resH + 2; 
    vertices.resize(nw * nh);
    vertices.resize(((nw - 1) * (nh - 2)) * 6);
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
/*
void parametric_shapes::createCuboid(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cuboid cuboid) {

}

void parametric_shapes::createSphere(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Sphere sphere) {

}

void parametric_shapes::createCylinder(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cylinder cylinder) {

}

void parametric_shapes::createCapsule(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Capsule capsule) {

}
*/