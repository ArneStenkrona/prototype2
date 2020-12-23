#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>

#include <string>   

#include <fstream>

ModelManager::ModelManager(const char* modelDirectory) {
    strcpy(m_modelDirectory, modelDirectory);
}

void ModelManager::getAnimatedModels(Model const * & models, size_t & nModels) {
    models = m_loadedAnimatedModels.data();
    nModels = m_loadedAnimatedModels.size();
}


void ModelManager::getBoneOffsets(uint32_t const * modelIndices,
                                  uint32_t * boneOffsets,
                                  size_t n) {
    size_t numBones = 0;
    for (size_t i = 0; i < n; ++i) {
        boneOffsets[i] = numBones;
        numBones += m_loadedAnimatedModels[modelIndices[i]].bones.size();
    }
}

void ModelManager::getSampledAnimation(float t, 
                                       prt::vector<uint32_t> const & modelIndices,
                                       prt::vector<uint32_t> const & animationIndices, 
                                       prt::vector<glm::mat4> & transforms) {
    size_t numBones = 0;
    for (auto & index : modelIndices) {
        numBones += m_loadedAnimatedModels[index].bones.size();
    }

    transforms.resize(numBones);
    size_t tIndex = 0;
    for (size_t i = 0; i < modelIndices.size(); ++i) {
        auto const & model = m_loadedAnimatedModels[modelIndices[i]];
        model.sampleAnimation(t, animationIndices[i], &transforms[tIndex]);
        tIndex += model.bones.size();
    }
}

void ModelManager::getSampledBlendedAnimation(uint32_t const * modelIndices,
                                              BlendedAnimation const * animationBlends, 
                                              prt::vector<glm::mat4> & transforms,
                                              size_t n) {
    size_t numBones = 0;
    for (size_t i = 0; i < n; ++i) {
        numBones += m_loadedAnimatedModels[modelIndices[i]].bones.size();
    }
    transforms.resize(numBones);

    size_t tIndex = 0;
    for (size_t i = 0; i < n; ++i) {
        auto const & model = m_loadedAnimatedModels[modelIndices[i]];
        model.blendAnimation(animationBlends[i].time, 
                             animationBlends[i].blendFactor, 
                             animationBlends[i].clipA, 
                             animationBlends[i].clipB, 
                             &transforms[tIndex]);
        tIndex += model.bones.size();
    }
}

void ModelManager::loadModels(char const * paths[], size_t count,
                              uint32_t * ids, bool animated, TextureManager & textureManager) {
    auto & models = animated ? m_loadedAnimatedModels : m_loadedNonAnimatedModels;
    
    char path[256];
    size_t dirLen = strlen(m_modelDirectory);
    strcpy(path, m_modelDirectory);
    char * subpath = path + dirLen;
    for (size_t i = 0; i < count; ++i) {
        if (m_pathToModelID.find(paths[i]) == m_pathToModelID.end()) {
            strcpy(subpath, paths[i]);
            size_t id = models.size();
            m_pathToModelID.insert(paths[i], id);
            ids[i] = id;

            models.push_back({});
            Model & model = models.back();
            model.load(path, animated, textureManager);
        } else {
            ids[i] = m_pathToModelID.find(paths[i])->value();
        }
    }
}

uint32_t ModelManager::getAnimationIndex(uint32_t modelIndex, char const * name) {
    return m_loadedAnimatedModels[modelIndex].getAnimationIndex(name);
}
