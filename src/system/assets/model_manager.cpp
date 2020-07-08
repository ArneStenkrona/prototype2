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

 void ModelManager::loadModels(char const * paths[], size_t count,
                               uint32_t * ids, bool animated) {
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
            model.load(path, animated);
        } else {
            ids[i] = m_pathToModelID.find(paths[i])->value();
        }
    }
}
