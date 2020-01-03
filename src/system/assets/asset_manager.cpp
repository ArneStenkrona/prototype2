#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(const char* assetDirectory)
    : _assetDirectory(assetDirectory),
      _modelManager((_assetDirectory + "models/").c_str()) {
}