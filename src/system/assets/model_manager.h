#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/game/component/component.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);

    inline void getNonAnimatedModels(Model const * & models, 
                                     size_t & nModels) const { models = m_loadedNonAnimatedModels.data();
                                                               nModels = m_loadedNonAnimatedModels.size(); }
    void getAnimatedModels(Model const * & models, size_t & nModels);
    void getBoneOffsets(uint32_t const * modelIndices,
                        uint32_t * boneOffsets,
                        size_t n);

    void getSampledAnimation(float t, 
                             prt::vector<uint32_t> const & modelIndices,
                             prt::vector<uint32_t> const & animationIndices, 
                             prt::vector<glm::mat4> & transforms);

    void getSampledBlendedAnimation(uint32_t const * modelIndices,
                                    BlendedAnimation const * animationBlends, 
                                    prt::vector<glm::mat4> & transforms,
                                    size_t n);

    inline Model const & getNonAnimatedModel(uint32_t modelID) const { return m_loadedNonAnimatedModels[modelID]; } 
    Model const & getAnimatedModel(uint32_t modelID) const { return m_loadedAnimatedModels[modelID]; }

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids, bool animated);

    uint32_t getAnimationIndex(uint32_t modelIndex, char const * name);

private:
    prt::hash_map<std::string, uint32_t> m_pathToModelID;
    char m_modelDirectory[256];
    prt::vector<Model> m_loadedNonAnimatedModels;
    prt::vector<Model> m_loadedAnimatedModels;
    ;
};

#endif