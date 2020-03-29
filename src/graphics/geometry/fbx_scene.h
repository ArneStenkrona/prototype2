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

namespace FBX {

struct Mesh {
    int64_t id;
    char name[256];
    prt::vector<glm::dvec3> vertices;
    prt::vector<int32_t> polygonVertexIndex;
    prt::vector<glm::dvec3> normals;
    prt::vector<glm::dvec2> uv;
    prt::vector<int32_t> uvIndex;
};

struct Model {
    int64_t id;
    char type[32];
    glm::dvec3 localTranslation;
    glm::dvec3 localRotation;
    glm::dvec3 localScaling;
};

struct Material {
    int64_t id;
    char name[256];
};

struct Texture {
    int64_t id;
    char relativeFilename[256];
};

struct AnimationCurveNode {
    double dx;
    double dy;
    double dz;
};

struct AnimationCurve {
    int64_t type;
    double defaultVal;
    int64_t keyTime[2];
    float kevalueFloat[2];
};

enum NODE_TYPE : int16_t {
    MESH,
    MODEL,
    MATERIAL,
    TEXTURE
};

struct TypedIndex {
NODE_TYPE type;
int16_t index;
};

class Scene {
public:
    Scene(char const* path);

    prt::vector<Mesh> const & getMeshes() { return meshes; }
    prt::vector<Model> const & getModels() { return models; }
    prt::vector<Material> const & getMaterials() { return materials; }
    prt::vector<Texture> const & getTextures() { return textures; }

    prt::vector<std::pair<int64_t, int64_t> > const & getConnections() { return connections; }


    struct GlobalSettings {
        int32_t upAxis = 1;
        int32_t upAxisSign = 1;
        int32_t frontAxis = 2;
        int32_t frontAxisSign = 1;
        int32_t coordAxis = 0;
        int32_t coordAxisSign = 1;
        int32_t originalUpAxis = 1;
        int32_t originalUpAxisSign = 1;
        /* TODO: add all global settings */
    };

    GlobalSettings getGlobalSettings() const  { return globalSettings; }

    prt::hash_map<int64_t, TypedIndex> const & getIdToIndex() const { return idToIndex; };    

    
private:
    void parseGlobalSettings(FBX::Document::Node const & node);
    void parseMesh(FBX::Document::Node const & node);
    void parseModel(FBX::Document::Node const & node);
    void parseMaterial(FBX::Document::Node const & node);
    void parseTexture(FBX::Document::Node const & node);
    void parseConnections(FBX::Document::Node const & node);

    prt::hash_map<int64_t, TypedIndex> idToIndex;

    prt::vector<Mesh> meshes;
    prt::vector<Model> models;
    prt::vector<Material> materials;
    prt::vector<Texture> textures;

    GlobalSettings globalSettings;

    prt::vector<std::pair<int64_t, int64_t> > connections;
};

};

#endif