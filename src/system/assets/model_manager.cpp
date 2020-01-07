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

ModelManager::ModelManager(const char* modelDirectory)
    : _modelDirectory(modelDirectory) {
    addModelPaths(modelDirectory);
}

void ModelManager::addModelPaths(const char* directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return;
    }
    std::string modelAssetPath = directory;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] != '.') {
            std::string modelName = std::string(entry->d_name);
            _modelPaths.insert(modelName, std::string(directory) + entry->d_name);
            _idToName.insert(nextID, modelName);
            _modelIDs.insert(modelName, nextID++);
        }
    }

    closedir(dir);
}

void ModelManager::getPaths(const prt::vector<uint32_t>& uniqueIDs, 
              prt::vector<std::string>& modelPaths) {
    modelPaths.resize(uniqueIDs.size());
    for (size_t i = 0; i < uniqueIDs.size(); i++) {
        std::string& name = _idToName[uniqueIDs[i]];
        modelPaths[i] = _modelPaths[name];
    }
}

uint32_t ModelManager::getModelID(std::string& name) {
    auto it = _modelIDs.find(name);
    assert((it != _modelIDs.end()));
    return it->value();
}

uint32_t ModelManager::getModelID(const char* name) {
    std::string name_str = std::string(name);
    return getModelID(name_str);
}

void ModelManager::loadModels(const prt::vector<uint32_t>& modelIDs, 
                              prt::vector<Model>& models,
                              prt::vector<Model>& colliderModels,
                              prt::vector<uint32_t>& colliderIDs) {
    prt::vector<std::string> modelPaths;
    getPaths(modelIDs, modelPaths);
    models.resize(modelIDs.size());
    colliderModels.resize(0);
    for (uint32_t i = 0; i < modelPaths.size(); i++) {
        models[i].loadOBJ((modelPaths[i] + "/model.obj").c_str());

        if (is_file_exist((modelPaths[i] + "/collider.obj").c_str())) {
            colliderModels.push_back({});
            colliderModels.back().loadOBJ((modelPaths[i] + "/collider.obj").c_str());
            colliderIDs.push_back(modelIDs[i]);
        }

        for (size_t mi = 0; mi < models[i]._meshes.size(); mi++) {
            std::string texPath =  modelPaths[i] + "/" + models[i]._meshes[mi]._name + "_diffuse.png";
            models[i]._meshes[mi]._texture.load(texPath.c_str());
        }
    }
}

void ModelManager::loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                                   prt::vector<Model>& models, 
                                   prt::vector<uint32_t>& modelIndices,
                                   prt::vector<Model>& colliderModels,
                                   prt::vector<uint32_t>& colliderIDs) {
    modelIndices.resize(modelIDs.size());                                  
    prt::hash_map<uint32_t, uint32_t> idToIndex;
    prt::vector<uint32_t> uniqueIDs;
    for (size_t i = 0; i < modelIDs.size(); i++) {
        const uint32_t& indx = modelIDs[i];
        if (idToIndex.find(indx) == idToIndex.end()) {
            idToIndex.insert(indx, uniqueIDs.size());
            uniqueIDs.push_back(indx);
        }
        modelIndices[i] = idToIndex[indx];
    }
    loadModels(uniqueIDs, models, colliderModels, colliderIDs);
}