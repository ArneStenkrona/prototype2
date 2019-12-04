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
    // indices
    uint32_t index = 0;
    for (uint32_t iw = 0; iw < nw - 1; iw++) {
         for (uint32_t ih = 0; ih < nh - 1; ih++) {
            // triangle 1
            indices[index++] = iw + ((ih + 1) * nw);
            indices[index++] = iw + 1 + (ih * nw);
            indices[index++] = iw + (ih * nw);
            // triangle 2
            indices[index++] = iw + ((ih + 1) * nw);
            indices[index++] = iw + 1 + ((ih + 1) * nw);
            indices[index++] = iw + 1 + (ih * nw);
        }
    }
}

void parametric_shapes::createCuboid(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cuboid cuboid) {
    float w = cuboid.width / 2;
    float h = cuboid.height / 2;
    float d = cuboid.depth / 2;
    vertices.resize(4*6);
    indices.resize(6*6);
    // I split each face to get correct normals
    /* face 1 */
    vertices[0].pos = glm::vec3{w, h, d};
    vertices[1].pos = glm::vec3{w, h, -d};
    vertices[2].pos = glm::vec3{-w, h, -d};
    vertices[3].pos = glm::vec3{-w, h, d};

    /* face 2 */
    vertices[7].pos = glm::vec3{w, -h, d};
    vertices[6].pos = glm::vec3{w, -h, -d};
    vertices[5].pos = glm::vec3{-w, -h, -d};
    vertices[4].pos = glm::vec3{-w, -h, d};

    /* face 3 */
    vertices[11].pos = glm::vec3{w, h, d};
    vertices[10].pos = glm::vec3{w, h, -d};
    vertices[9].pos = glm::vec3{w, -h, -d};
    vertices[8].pos = glm::vec3{w, -h, d};

    /* face 4 */
    vertices[12].pos = glm::vec3{-w, h, d};
    vertices[13].pos = glm::vec3{-w, h, -d};
    vertices[14].pos = glm::vec3{-w, -h, -d};
    vertices[15].pos = glm::vec3{-w, -h, d};

    /* face 5 */
    vertices[19].pos = glm::vec3{w, h, d};
    vertices[18].pos = glm::vec3{w, -h, d};
    vertices[17].pos = glm::vec3{-w, -h, d};
    vertices[16].pos = glm::vec3{-w, h, d};

    /* face 6 */
    vertices[20].pos = glm::vec3{w, h, -d};
    vertices[21].pos = glm::vec3{w, -h, -d};
    vertices[22].pos = glm::vec3{-w, -h, -d};
    vertices[23].pos = glm::vec3{-w, h, -d};

    glm::vec3 normals[6] = { glm::vec3{ 0.0f, 1.0f, 0.0f},
                             glm::vec3{ 0.0f, -1.0f, 0.0f},
                             glm::vec3{ 1.0f, 0.0f, 0.0f},
                             glm::vec3{ -1.0f, 0.0f, 0.0f},
                             glm::vec3{ 0.0f, 0.0f, 1.0f},
                             glm::vec3{ 0.0f, 0.0f, -1.0f}};

    uint32_t index = 0;
    for (uint32_t i = 0; i < 6; i++) {
        indices[index++] = (i * 4);
        indices[index++] = (i * 4) + 1;
        indices[index++] = (i * 4) + 2;

        indices[index++] = (i * 4) + 2;
        indices[index++] = (i * 4) + 3;
        indices[index++] = (i * 4);

        vertices[i * 4].normal = normals[i];
        vertices[i * 4 + 1].normal = normals[i];
        vertices[i * 4 + 2].normal = normals[i];
        vertices[i * 4 + 3].normal = normals[i];

        vertices[i * 4].texCoord = glm::vec2(1.0f, 0.0f);
        vertices[i * 4 + 1].texCoord = glm::vec2(0.0f, 0.0f);
        vertices[i * 4 + 2].texCoord = glm::vec2(0.0f, 1.0f);
        vertices[i * 4 + 3].texCoord = glm::vec2(1.0f, 1.0f);
    }
}

void parametric_shapes::createSphere(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Sphere sphere) {
    uint32_t ntheta = sphere.res + 2;
    uint32_t nphi = ntheta/2;
    vertices.resize(nphi * ntheta);
    indices.resize((ntheta * (nphi - 1)) * 6);
    uint32_t vi = 0;
    for (uint32_t iphi = 0; iphi < nphi; iphi++) {
        float fphi = (float(iphi) / float(nphi-1));
        float phi = fphi * glm::pi<float>();
        for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
            float ftheta = (float(itheta) / float(ntheta-1));
            float theta = ftheta * 2 * glm::pi<float>();
            Vertex v; 
            v.pos = glm::vec3{
                              sphere.radius * glm::sin(theta) * glm::sin(phi),
                              sphere.radius * glm::cos(phi),
                              sphere.radius * glm::cos(theta) * glm::sin(phi)
                              };                
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2{ftheta, fphi};
            vertices[vi++] = v;
        }
    }

    uint32_t index = 0;
    // indices
    // bottom cap
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
    // top cap
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = (nphi - 2) * ntheta + ((itheta + 1) % ntheta);
        indices[index++] = (nphi - 2) * ntheta + itheta;
        indices[index++] = (nphi - 1) * ntheta + itheta;
    }
}


void parametric_shapes::createCylinder(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Cylinder cylinder) {
    uint32_t ntheta = cylinder.res + 2;
    vertices.resize(3 * 4 * ntheta);
    indices.resize(2 * 6 * ntheta);
    uint32_t vi = 0;
    /* middle */
    /* bottom */
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        float theta = ftheta * 2 * glm::pi<float>();
        Vertex v; 
        v.pos = glm::vec3{ 
                           cylinder.radius * glm::sin(theta),
                           0.0f,
                           cylinder.radius * glm::cos(theta)
                           };      
        v.normal = glm::vec3{ glm::sin(theta), 0.0f, glm::cos(theta) };                             
        v.texCoord = glm::vec2{ftheta, 1.0f};
        vertices[vi++] = v;
    }
    /* top */
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        float theta = ftheta * 2 * glm::pi<float>();
        Vertex v; 
        v.pos = glm::vec3{ 
                           cylinder.radius * glm::sin(theta), 
                           cylinder.height,
                           cylinder.radius * glm::cos(theta)
                           };      
        v.normal = glm::vec3{ glm::sin(theta), 0.0f, glm::cos(theta) };                   
        v.texCoord = glm::vec2{ftheta, 0.0f};
        vertices[vi++] = v;
    }
    /* caps */
    /* bottom */
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        Vertex v; 
        v.pos = glm::vec3{ 
                           0.0f,
                           0.0f,
                           0.0f,
                           };      
        v.normal = glm::vec3{ 0.0f, -1.0f, 0.0f };                             
        v.texCoord = glm::vec2{ftheta, 1.0f};
        vertices[vi++] = v;
    }
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        float theta = ftheta * 2 * glm::pi<float>();
        Vertex v; 
        v.pos = glm::vec3{ 
                           cylinder.radius * glm::sin(theta),
                           0.0f,
                           cylinder.radius * glm::cos(theta)
                           };      
        v.normal = glm::vec3{ 0.0f, -1.0f, 0.0f };                                                          
        v.texCoord = glm::vec2{ftheta, 0.0f};
        vertices[vi++] = v;
    }
    /* top */
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        float theta = ftheta * 2 * glm::pi<float>();
        Vertex v; 
        v.pos = glm::vec3{ 
                           cylinder.radius * glm::sin(theta), 
                           cylinder.height,
                           cylinder.radius * glm::cos(theta)
                           };      
        v.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };                                                
        v.texCoord = glm::vec2{ftheta, 0.0f};
        vertices[vi++] = v;
    }
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        float ftheta = (float(itheta) / float(ntheta-1));
        Vertex v; 
        v.pos = glm::vec3{ 
                           0.0f, 
                           cylinder.height,
                           0.0f
                           };      
        v.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };                                               
        v.texCoord = glm::vec2{ftheta, 1.0f};
        vertices[vi++] = v;
    }

    /* indices */
    /* middle */
    uint32_t index = 0;
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        // triangle 1
        indices[index++] = itheta;
        indices[index++] = ((itheta + 1) % ntheta);
        indices[index++] = ((itheta + 1) % ntheta) + ntheta;
        // triangle 2
        indices[index++] = ((itheta + 1) % ntheta) + ntheta;
        indices[index++] = itheta + ntheta;
        indices[index++] = itheta;
    }
    /* caps */
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = ((itheta + 1) % ntheta) + ntheta + 2 * ntheta;
        indices[index++] = itheta + ntheta + 2 * ntheta;
        indices[index++] = itheta + 2 * ntheta;
    }
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = itheta + 4 * ntheta;
        indices[index++] = ((itheta + 1) % ntheta) + 4 * ntheta;
        indices[index++] = ((itheta + 1) % ntheta) + ntheta + 4 * ntheta;
    }
}

void parametric_shapes::createCapsule(prt::vector<Vertex>& vertices, prt::vector<uint32_t>& indices, Capsule capsule) {
    uint32_t ntheta = capsule.res + 2;
    uint32_t nphi = ntheta/2;
    vertices.resize(nphi * ntheta);
    indices.resize((ntheta * (nphi - 1)) * 6);
    float h = std::max(0.0f, capsule.height - 2 * capsule.radius);
    uint32_t vi = 0;
    for (uint32_t iphi = 0; iphi < nphi / 2; iphi++) {
        float fphi = (float(iphi) / float(nphi-1));
        float phi = fphi * glm::pi<float>();
        for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
            float ftheta = (float(itheta) / float(ntheta-1));
            float theta = ftheta * 2 * glm::pi<float>();
            Vertex v; 
            v.pos = glm::vec3{
                              capsule.radius * glm::sin(theta) * glm::sin(phi),
                              capsule.radius * glm::cos(phi) + h,
                              capsule.radius * glm::cos(theta) * glm::sin(phi)
                              };                
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2{ftheta, fphi};
            vertices[vi++] = v;
        }
    }
    for (uint32_t iphi = nphi / 2; iphi < nphi; iphi++) {
        float fphi = (float(iphi) / float(nphi-1));
        float phi = fphi * glm::pi<float>();
        for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
            float ftheta = (float(itheta) / float(ntheta-1));
            float theta = ftheta * 2 * glm::pi<float>();
            Vertex v; 
            v.pos = glm::vec3{
                              capsule.radius * glm::sin(theta) * glm::sin(phi),
                              capsule.radius * glm::cos(phi),
                              capsule.radius * glm::cos(theta) * glm::sin(phi)
                              };                
            v.normal = glm::normalize(v.pos);
            v.texCoord = glm::vec2{ftheta, fphi};
            vertices[vi++] = v;
        }
    }

    uint32_t index = 0;
    // indices
    // bottom cap
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
    // top cap
    for (uint32_t itheta = 0; itheta < ntheta; itheta++) {
        indices[index++] = (nphi - 2) * ntheta + ((itheta + 1) % ntheta);
        indices[index++] = (nphi - 2) * ntheta + itheta;
        indices[index++] = (nphi - 1) * ntheta + itheta;
    }
}