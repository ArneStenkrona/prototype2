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
    prt::vector<std::string> subdirectories;
    std::string modelAssetPath = directory;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_name[0] != '.') {
            if(entry->d_type == DT_DIR) { /* only deal with regular file */
                char subdirectory[512];
                strcpy(subdirectory, directory);
                strcat(subdirectory, entry->d_name);
                subdirectories.push_back(subdirectory);
            } else if (entry->d_type == DT_REG) {
                const char *dot = strrchr(entry->d_name,'.');
                if (dot && (strcmp(dot, ".obj") == 0 || strcmp(dot, ".fbx") == 0)) {
                    std::string path = (std::string(directory)) + "/" + entry->d_name;
                   
                    size_t nameLength = path.size() - _modelDirectory.size();
                    std::string modelName = path.substr(_modelDirectory.size(), nameLength);
                    _modelPaths.insert(modelName, path);
                    _idToName.insert(nextID, modelName);
                    _modelIDs.insert(modelName, nextID++);
                }
            }
        }
    }

    closedir(dir);
    for (size_t i = 0; i < subdirectories.size(); i++) {
        addModelPaths(subdirectories[i].c_str());
    }
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
                              prt::vector<Model>& models) {
    prt::vector<std::string> modelPaths;
    getPaths(modelIDs, modelPaths);
    models.resize(modelIDs.size());
    for (uint32_t i = 0; i < modelPaths.size(); i++) {
        const char *dot = strrchr(modelPaths[i].c_str(),'.');
        assert(dot && "Model name must end with file extension!");
        if (strcmp(dot, ".obj") == 0) {
            models[i].loadOBJ(modelPaths[i].c_str());
        } else if (strcmp(dot, ".fbx") == 0){
            models[i].loadFBX(modelPaths[i].c_str());
        }
    }
}

void ModelManager::loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                                   prt::vector<Model>& models, 
                                   prt::vector<uint32_t>& modelIndices) {
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
    loadModels(uniqueIDs, models);
}