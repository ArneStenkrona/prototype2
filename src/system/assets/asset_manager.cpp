#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(const char* assetDirectory)
    :_modelManager((std::string(assetDirectory) + "/models/").c_str()) {
}