#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/graphics/geometry/model.h"

class ModelManager {
public:
    ModelManager(const char* directory);

    inline void getNonAnimatedModels(Model const * & models, 
                                     size_t & nModels) const { models = m_loadedNonAnimatedModels.data();
                                                               nModels = m_loadedNonAnimatedModels.size(); }
    void getAnimatedModels(Model const * & models, uint32_t const * & boneOffsets, size_t & nModels);

    void getSampledAnimation(float t, 
                             prt::vector<uint32_t> const & modelIndices,
                             prt::vector<uint32_t> const & animationIndices, 
                             prt::vector<glm::mat4> & transforms);

    struct AnimationBlend {
        float time;
        uint32_t animationIndexA;
        uint32_t animationIndexB;
        float blendFactor;
    };
    void getSampledBlendedAnimation(prt::vector<uint32_t> const & modelIndices,
                                    prt::vector<AnimationBlend> const & animationBlends, 
                                    prt::vector<glm::mat4> & transforms);

    inline Model const & getNonAnimatedModel(uint32_t modelID) const { return m_loadedNonAnimatedModels[modelID]; } 
    Model const & getAnimatedModel(uint32_t modelID, uint32_t & boneOffset) const;

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids, bool animated);

    uint32_t getAnimationIndex(uint32_t modelIndex, char const * name);

private:
    prt::hash_map<std::string, uint32_t> m_pathToModelID;
    char m_modelDirectory[256];
    prt::vector<Model> m_loadedNonAnimatedModels;
    struct {
        prt::vector<Model> models;
        prt::vector<uint32_t> boneOffsets;
    } m_loadedAnimatedModels;
};

#endif