#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>

#include <string>   

#include <fstream>

bool ModelManager::defAlreadyLoaded = false;


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

void ModelManager::sampleAnimation(ModelID const * modelIDs,
                                              AnimationComponent * animationComponents, 
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
        if (animationComponents[i].blendFactor <= 0.0f) {
            model.sampleAnimation(animationComponents[i].clipA, &transforms[tIndex]);
        } else if (animationComponents[i].blendFactor >= 1.0f) {
            model.sampleAnimation(animationComponents[i].clipB, &transforms[tIndex]);
        } else {
            model.blendAnimation(animationComponents[i].clipA, 
                                 animationComponents[i].clipB, 
                                 animationComponents[i].blendFactor,
                                 &transforms[tIndex]);
        }
        tIndex += model.bones.size();
    }
}

// TODO: Fix so that no conflict arises if model
// has been loaded as both animated and non-animated 
ModelID ModelManager::loadModel(char const * path,
                                bool animated, bool & alreadyLoaded) {
    ModelID id;
    
    char fullPath[256];
    size_t dirLen = strlen(m_modelDirectory);
    strcpy(fullPath, m_modelDirectory);
    char * subpath = fullPath + dirLen;

    alreadyLoaded = m_pathToModelID.find(path) != m_pathToModelID.end();

    if (!alreadyLoaded) {
        strcpy(subpath, path);
        id = m_loadedModels.size();

        m_loadedModels.push_back(Model{fullPath});
        Model & model = m_loadedModels.back();

        if (!model.load(animated, m_textureManager)) {
            m_loadedModels.pop_back();
            id = -1;
        } else {
            m_pathToModelID.insert(path, id);
        }
    } else {
        // TODO: handle animation loading
        id = m_pathToModelID.find(path)->value();
    }

    return id;
}

uint32_t ModelManager::getAnimationIndex(ModelID modelID, char const * name) {
    return m_loadedModels[modelID].getAnimationIndex(name);
}
