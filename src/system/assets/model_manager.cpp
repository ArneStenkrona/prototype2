#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>

#include <string>   

#include <fstream>

ModelManager::ModelManager(const char* modelDirectory, TextureManager & textureManager) 
    : m_textureManager(textureManager) {
    strcpy(m_modelDirectory, modelDirectory);
}

void ModelManager::getBoneOffsets(ModelID const * modelIDs,
                                  uint32_t * boneOffsets,
                                  size_t n) {
    size_t numBones = 0;
    for (size_t i = 0; i < n; ++i) {
        boneOffsets[i] = numBones;
        numBones += m_loadedModels[modelIDs[i]].bones.size();
    }
}

void ModelManager::getSampledAnimation(float t, 
                                       prt::vector<ModelID> const & modelIDs,
                                       prt::vector<uint32_t> const & animationIndices, 
                                       prt::vector<glm::mat4> & transforms) {
    size_t numBones = 0;
    for (auto & index : modelIDs) {
        numBones += m_loadedModels[index].bones.size();
    }

    transforms.resize(numBones);
    size_t tIndex = 0;
    for (size_t i = 0; i < modelIDs.size(); ++i) {
        auto const & model = m_loadedModels[modelIDs[i]];
        model.sampleAnimation(t, animationIndices[i], &transforms[tIndex]);
        tIndex += model.bones.size();
    }
}

void ModelManager::getSampledBlendedAnimation(ModelID const * modelIDs,
                                              BlendedAnimation const * animationBlends, 
                                              prt::vector<glm::mat4> & transforms,
                                              size_t n) {
    size_t numBones = 0;
    for (size_t i = 0; i < n; ++i) {
        numBones += m_loadedModels[modelIDs[i]].bones.size();
    }
    transforms.resize(numBones);

    size_t tIndex = 0;
    for (size_t i = 0; i < n; ++i) {
        auto const & model = m_loadedModels[modelIDs[i]];
        model.blendAnimation(animationBlends[i].time, 
                             animationBlends[i].blendFactor, 
                             animationBlends[i].clipA, 
                             animationBlends[i].clipB, 
                             &transforms[tIndex]);
        tIndex += model.bones.size();
    }
}

ModelID ModelManager::loadModel(char * path,
                                bool animated) {
    ModelID id;
    
    char fullPath[256];
    size_t dirLen = strlen(m_modelDirectory);
    strcpy(fullPath, m_modelDirectory);
    char * subpath = fullPath + dirLen;

    if (m_pathToModelID.find(path) == m_pathToModelID.end()) {
        strcpy(subpath, path);
        id = m_loadedModels.size();
        m_pathToModelID.insert(path, id);

        m_loadedModels.push_back(Model{fullPath});
        m_loadedModels.back().load(animated, m_textureManager);
    } else {
        id = m_pathToModelID.find(path)->value();
    }

    return id;
}

void ModelManager::loadModels(char const * paths[], size_t count,
                              ModelID * ids, bool animated) {    
    char path[256];
    size_t dirLen = strlen(m_modelDirectory);
    strcpy(path, m_modelDirectory);
    char * subpath = path + dirLen;
    for (size_t i = 0; i < count; ++i) {
        if (m_pathToModelID.find(paths[i]) == m_pathToModelID.end()) {
            strcpy(subpath, paths[i]);
            size_t id = m_loadedModels.size();
            m_pathToModelID.insert(paths[i], id);
            ids[i] = id;

            m_loadedModels.push_back(Model{path});
            m_loadedModels.back().load(animated, m_textureManager);
        } else {
            ids[i] = m_pathToModelID.find(paths[i])->value();
        }
    }
}

uint32_t ModelManager::getAnimationIndex(ModelID modelID, char const * name) {
    return m_loadedModels[modelID].getAnimationIndex(name);
}
