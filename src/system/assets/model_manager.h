#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);

    inline void getModels(Model const * & models, size_t & nModels) const { models = m_loadedModels.data(); 
                                                                            nModels = m_loadedModels.size(); }
    inline Model const & getModel(uint32_t modelID) const { return m_loadedModels[modelID]; } 

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids);

private:
    prt::hash_map<std::string, uint32_t> m_pathToModelID;
    char m_modelDirectory[256];
    prt::vector<Model> m_loadedModels;
};

#endif