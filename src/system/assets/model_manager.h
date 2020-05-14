#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);

    // void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
    //                      prt::vector<Model>& models, 
    //                      prt::vector<uint32_t>& modelIndices);

    inline void getModels(Model const * & models, size_t & nModels) const { models = m_loadedModels.data(); 
                                                                            nModels = m_loadedModels.size(); }
    inline Model const & getModel(uint32_t modelID) const { return m_loadedModels[modelID]; } 


    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids);

    // void loadModels(prt::vector<uint32_t> const & modelIDs,
    //                 prt::vector<uint32_t> & modelIndices);

    // uint32_t getModelID(std::string& name);
    // uint32_t getModelID(const char* name);

    // static constexpr uint32_t UNDEFINED_MODEL = -1;
    // static constexpr uint32_t DEFAULT_MODEL = 0;

private:
    // prt::hash_map<std::string, std::string> m_modelPaths;
    // prt::hash_map<std::string, uint32_t> m_modelIDs;
    // prt::hash_map<uint32_t, std::string> m_idToName;
    prt::hash_map<std::string, uint32_t> m_pathToModelID;

    // std::string m_modelDirectory;
    char m_modelDirectory[256];
    // uint32_t m_nextID = 0;

    prt::vector<Model> m_loadedModels;

    // void addModelPaths(const char* directory);
    
    // void loadOBJPaths(const char* directory);

    // void getPaths(const prt::vector<uint32_t>& modelIDs, 
    //               prt::vector<std::string>& modelPaths);

    // void loadModels(const prt::vector<uint32_t>& modelIDs, 
    //                 prt::vector<Model>& models);

};

#endif