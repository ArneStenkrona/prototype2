#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/graphics/geometry/model.h"

#include <dirent.h>

/*typedef char ModelName[128];

namespace std {
    template<> struct hash<ModelName> {
        size_t operator()(ModelName const& modelName) const {
            size_t result = 0;
            const size_t prime = 31;
            for (size_t i = 0; i < 128; ++i) {
                result = modelName[i] + (result * prime);
            }
            return result;
        }
    };
}*/

class ModelManager {
public:
    ModelManager(const char* directory);
    void insertQuads(prt::vector<parametric_shapes::Quad>& quads);
    void insertSpheres(prt::vector<parametric_shapes::Sphere>& spheres);
    void insertCylinders(prt::vector<parametric_shapes::Cylinder>& cylinders);

    void loadModels(prt::vector<Model>& models);
    void loadMeshes(const char* modelPath, prt::vector<Mesh>& meshes,
                                          prt::vector<Vertex>& vertexBuffer,
                                          prt::vector<uint32_t>& indexBuffer);

    void loadTextures(const char* texturePath, Texture& texture);


    void getPaths(prt::vector<std::string>& modelPaths, prt::vector<std::string>& texturePaths);
    uint32_t getModelID(std::string& name);
    uint32_t getModelID(const char* name);

    static constexpr uint32_t UNDEFINED_MODEL = -1;
    static constexpr uint32_t DEFAULT_MODEL = 0;
    
private:
    prt::hash_map<std::string, std::string> _modelPaths;
    prt::hash_map<std::string, std::string> _texturePaths;
    prt::hash_map<std::string, uint32_t> _modelIDs;

    // Parametrics
    prt::vector<parametric_shapes::Quad> _quads;
    prt::vector<parametric_shapes::Sphere> _spheres;
    prt::vector<parametric_shapes::Cylinder> _cylinders;

    std::string _directory;
    uint16_t nextID = 0;
    
    void loadPersistent(const char* directory);
};

#endif