#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/game/level/level_map.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/graphics/geometry/model.h"

#include <dirent.h>

class ModelManager {
public:
    ModelManager(const char* directory);


    void loadModels(const prt::vector<uint32_t>& uniqueIDs, prt::vector<Model>& models);
    void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                         prt::vector<Model>& models, 
                         prt::vector<uint32_t>& modelIndices);

    uint32_t getModelID(std::string& name);
    uint32_t getModelID(const char* name);

    static constexpr uint32_t UNDEFINED_MODEL = -1;
    static constexpr uint32_t DEFAULT_MODEL = 0;
    
    enum MODEL_TYPE {
        OBJ,
        SKYBOX,
        TOTAL_MODEL_TYPES
    };
    struct ModelPath {
        MODEL_TYPE type;
        std::string path;

        bool operator==(const ModelPath& other) const {
            return type == other.type && path.compare(other.path) == 0;
        }

        bool operator!=(const ModelPath& other) const {
            return !(*this == other);
        }
    };

private:

    prt::hash_map<std::string, ModelPath> _modelPaths;
    prt::hash_map<std::string, uint32_t> _modelIDs;
    prt::hash_map<uint32_t, std::string> _idToName;

    std::string _modelDirectory;
    uint16_t nextID = 0;

    void addModelPaths(const char* directory, const char* postfix, MODEL_TYPE type);
    
    void loadOBJPaths(const char* directory);
    void loadOBJ(const char* path, Model& model);

    void loadSkyboxPaths(const char* directory);
    void loadSkybox(const char* path, Model& model);

    void getPaths(const prt::vector<uint32_t>& IDs, 
                  prt::vector<ModelPath>& modelPaths);
};

namespace std {
template<> struct hash<ModelManager::ModelPath> {
        size_t operator()(ModelManager::ModelPath const& modelPath) const {
            return hash<std::string>()(modelPath.path);
        }
    };
}

#endif