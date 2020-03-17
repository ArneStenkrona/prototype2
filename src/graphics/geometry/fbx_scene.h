#ifndef FBX_SCENE_H
#define FBX_SCENE_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "src/container/vector.h"
/*
Geometry { (int64: 190771707) (string: "Geometry::Cube") (string: "Mesh") }
            Properties70 { }
            GeometryVersion { (int32: 124) }
            Vertices { (double[24]) }
            PolygonVertexIndex { (int32[24]) }
            Edges { (int32[12]) }
            LayerElementNormal { (int32: 0) }
                Version { (int32: 101) }
                Name { (string: "") }
                MappingInformationType { (string: "ByPolygonVertex") }
                ReferenceInformationType { (string: "Direct") }
                Normals { (double[72]) }
            LayerElementUV { (int32: 0) }
                Version { (int32: 101) }
                Name { (string: "UVMap") }
                MappingInformationType { (string: "ByPolygonVertex") }
                ReferenceInformationType { (string: "IndexToDirect") }
                UV { (double[28]) }
                UVIndex { (int32[24]) }
            Layer { (int32: 0) }
                Version { (int32: 100) }
                LayerElement { }
                    Type { (string: "LayerElementNormal") }
                    TypedIndex { (int32: 0) }
                LayerElement { }
                    Type { (string: "LayerElementUV") }
                    TypedIndex { (int32: 0) }
                    */

struct FBX_Mesh {
    int64_t id;
    char name;
    prt::vector<glm::dvec3> vertices;
    prt::vector<int32_t> polygonVertexIndex;
    prt::vector<glm::dvec3> normals;
    prt::vector<glm::dvec2> uv;
    prt::vector<int32_t> uvIndex;
};

class FBX_Scene {
public:
    FBX_Scene(char const* path);
private:
};

#endif