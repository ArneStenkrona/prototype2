#include "model_manager.h"

#include <dirent.h>

ModelManager::ModelManager(const char* directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp (entry->d_name, "MODEL_", strlen("MODEL_")) == 0) {
            auto modelDir = std::string(entry->d_name);
            auto modelName = modelDir.substr(strlen("MODEL_"));
            _modelPaths.insert(modelName, directory + modelDir + "/");
        }
    }

    closedir(dir);
}

void ModelManager::getPaths(prt::vector<std::string>& v) {
    for (auto it = _modelPaths.begin(); it != _modelPaths.end(); it++) {
        v.push_back(it->value());
    }
}