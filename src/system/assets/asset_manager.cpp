#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(const char* assetDirectory)
    :_modelManager((std::string(assetDirectory) + "/models/").c_str()) {
    struct dirent *entry;
    DIR *dir = opendir(assetDirectory);
    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strncmp(entry->d_name, "MODEL_", strlen("MODEL_")) == 0) {

        }
    }

    closedir(dir);
}