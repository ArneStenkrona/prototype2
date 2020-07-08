#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);

    inline void getModels(Model const * & models, 
                          size_t & nModels,
                          bool animated = false) const { models = animated ? m_loadedAnimatedModels.data() : 
                                                                             m_loadedNonAnimatedModels.data();
                                                         nModels = animated ? m_loadedAnimatedModels.size() :
                                                                              m_loadedNonAnimatedModels.size(); }
    inline Model const & getModel(uint32_t modelID, 
                                  bool animated = false) const { return animated ? m_loadedAnimatedModels[modelID] : 
                                                                                   m_loadedNonAnimatedModels[modelID]; } 

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids, bool animated);

private:
    prt::hash_map<std::string, uint32_t> m_pathToModelID;
    char m_modelDirectory[256];
    prt::vector<Model> m_loadedNonAnimatedModels;
    prt::vector<Model> m_loadedAnimatedModels;
};

#endif