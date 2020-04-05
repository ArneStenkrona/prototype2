#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);



    void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                         prt::vector<Model>& models, 
                         prt::vector<uint32_t>& modelIndices);

    uint32_t getModelID(std::string& name);
    uint32_t getModelID(const char* name);

    static constexpr uint32_t UNDEFINED_MODEL = -1;
    static constexpr uint32_t DEFAULT_MODEL = 0;

private:
    prt::hash_map<std::string, std::string> _modelPaths;
    prt::hash_map<std::string, uint32_t> _modelIDs;
    prt::hash_map<uint32_t, std::string> _idToName;

    std::string _modelDirectory;
    uint32_t nextID = 0;

    void addModelPaths(const char* directory);
    
    void loadOBJPaths(const char* directory);

    void getPaths(const prt::vector<uint32_t>& modelIDs, 
                  prt::vector<std::string>& modelPaths);

    void loadModels(const prt::vector<uint32_t>& modelIDs, 
                    prt::vector<Model>& models);
};

#endif