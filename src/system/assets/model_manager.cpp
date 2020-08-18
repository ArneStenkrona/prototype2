#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>

#include <string>   

#include <fstream>

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

ModelManager::ModelManager(const char* modelDirectory) {
    strcpy(m_modelDirectory, modelDirectory);
}
void ModelManager::getAnimatedModels(Model const * & models, uint32_t const * & boneOffsets, size_t & nModels) {
    models = m_loadedAnimatedModels.models.data();
    boneOffsets = m_loadedAnimatedModels.boneOffsets.data();
    nModels = m_loadedAnimatedModels.models.size();
}

// void ModelManager::getBoneOffsets(prt::vector<uint32_t> offsets) {
//     offsets.resize(m_loadedAnimatedModels.size());
//     size_t currentOffset = 0;
//     for (size_t i = 0; i < m_loadedAnimatedModels.size(); ++i) {
//         offsets[i] = currentOffset;
//         currentOffset += m_loadedAnimatedModels[i].bones.size();
//     }
// }

void ModelManager::getSampledAnimation(float t, 
                                       prt::vector<uint32_t> const & modelIndices,
                                       prt::vector<uint32_t> const & animationIndices, 
                                       prt::vector<glm::mat4> & transforms) {
    size_t numBones = 0;
    for (auto & index : modelIndices) {
        numBones += m_loadedAnimatedModels.models[index].bones.size();
    }

    transforms.resize(numBones);
    size_t tIndex = 0;
    for (size_t i = 0; i < modelIndices.size(); ++i) {
        auto const & model = m_loadedAnimatedModels.models[modelIndices[i]];
        model.sampleAnimation(t, animationIndices[i], &transforms[tIndex]);
        tIndex += model.bones.size();
    }
}

Model const & ModelManager::getAnimatedModel(uint32_t modelID, uint32_t & boneOffset) const {
    boneOffset = m_loadedAnimatedModels.boneOffsets[modelID];
    return m_loadedAnimatedModels.models[modelID];
}


void ModelManager::loadModels(char const * paths[], size_t count,
                              uint32_t * ids, bool animated) {
    auto & models = animated ? m_loadedAnimatedModels.models : m_loadedNonAnimatedModels;
    
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
            model.load(path, animated);

            if (animated) { 
                // save the bone offset
                uint32_t offset = id > 0 ? models[id - 1].bones.size() : 0;
                m_loadedAnimatedModels.boneOffsets.push_back(offset);
            }
        } else {
            ids[i] = m_pathToModelID.find(paths[i])->value();
        }
    }
}

uint32_t ModelManager::getAnimationIndex(uint32_t modelIndex, char const * name) {
    return m_loadedAnimatedModels.models[modelIndex].getAnimationIndex(name);
}
