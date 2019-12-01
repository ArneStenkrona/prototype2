#include "model_manager.h"

#include <dirent.h>
#include <fstream>

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

ModelManager::ModelManager(const char* directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return;
    }

    uint16_t nextID = 0;
    // Add default
    std::string modelAssetPath = std::string("P/") + directory;
    _modelPaths.insert("DEFAULT", modelAssetPath + "DEFAULT/model.obj");
    _texturePaths.insert("DEFAULT", modelAssetPath + "DEFAULT/diffuse.png");
    _modelIDs.insert("DEFAULT", nextID++);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp (entry->d_name, "MODEL_", strlen("MODEL_")) == 0) {
            auto modelDir = std::string(entry->d_name);
            std::string modelName = modelDir.substr(strlen("MODEL_"));
            assert((modelName.compare("DEFAULT") != 0));
            _modelPaths.insert(modelName, modelAssetPath + modelDir + "/model.obj");
            auto texturePath = is_file_exist((directory + modelDir + "/diffuse.png").c_str()) ?
                               modelAssetPath + modelDir + "/diffuse.png" : modelAssetPath + "DEFAULT/diffuse.png";
            _texturePaths.insert(modelName, texturePath);
            _modelIDs.insert(modelName, nextID++);
        }
    }

    closedir(dir);
}

void ModelManager::getPaths(prt::vector<std::string>& modelPaths, prt::vector<std::string>& texturePaths) {
    modelPaths.resize(_modelPaths.size());
    for (auto it = _modelPaths.begin(); it != _modelPaths.end(); it++) {
        uint32_t index = _modelIDs[it->key()];
        modelPaths[index] = it->value();
    }
    texturePaths.resize(_texturePaths.size());
    for (auto it = _texturePaths.begin(); it != _texturePaths.end(); it++) {
        uint32_t index = _modelIDs[it->key()];
        texturePaths[index] = it->value();
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