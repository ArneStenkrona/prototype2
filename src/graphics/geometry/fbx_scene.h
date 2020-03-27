#ifndef FBX_SCENE_H
#define FBX_SCENE_H

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>

#include "src/graphics/geometry/fbx_document.h"

#include "src/container/vector.h"
#include "src/container/hash_map.h"

#include <utility>

struct FBX_Mesh {
    int64_t id;
    char name[256];
    prt::vector<glm::dvec3> vertices;
    prt::vector<int32_t> polygonVertexIndex;
    prt::vector<glm::dvec3> normals;
    prt::vector<glm::dvec2> uv;
    prt::vector<int32_t> uvIndex;
};

struct FBX_Model {
    int64_t id;
    glm::dvec3 localTranslation;
    glm::dvec3 localRotation;
    glm::dvec3 localScaling;
};

struct FBX_Material {
    int64_t id;
    char name[256];
};

struct FBX_Texture {
    int64_t id;
    char relativeFilename[256];
};

enum FBX_TYPE : int16_t {
    MESH,
    MODEL,
    MATERIAL,
    TEXTURE
};

class FBX_Scene {
public:
    FBX_Scene(char const* path);

    prt::vector<FBX_Mesh> const & getMeshes() { return meshes; }
    prt::vector<FBX_Model> const & getModels() { return models; }
    prt::vector<FBX_Material> const & getMaterials() { return materials; }
    prt::vector<FBX_Texture> const & getTextures() { return textures; }

    prt::vector<std::pair<int64_t, int64_t> > const & getConnections() { return connections; }

    struct TypedIndex {
    FBX_TYPE type;
    int16_t index;
    };

    prt::hash_map<int64_t, TypedIndex> const & getIdToIndex() const { return idToIndex; };    

    
private:
    void parseMesh(FBX_Document::FBX_Node const & node);
    void parseModel(FBX_Document::FBX_Node const & node);
    void parseMaterial(FBX_Document::FBX_Node const & node);
    void parseTexture(FBX_Document::FBX_Node const & node);
    void parseConnections(FBX_Document::FBX_Node const & node);

    prt::hash_map<int64_t, TypedIndex> idToIndex;

    prt::vector<FBX_Mesh> meshes;
    prt::vector<FBX_Model> models;
    prt::vector<FBX_Material> materials;
    prt::vector<FBX_Texture> textures;

    prt::vector<std::pair<int64_t, int64_t> > connections;
};

#endif