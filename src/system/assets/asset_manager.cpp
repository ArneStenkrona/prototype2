#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(const char* assetDirectory)
    : _assetDirectory(assetDirectory),
      _modelManager((_assetDirectory + "models/").c_str()) {
}

void AssetManager::loadCubeMap(const char* name, prt::array<Texture, 6>& cubeMap) {
  std::string path = (_assetDirectory + "textures/skybox/") + name;
  cubeMap[0].load((std::string(path) + "/front.png").c_str());
  cubeMap[1].load((std::string(path) + "/back.png").c_str());
  cubeMap[2].load((std::string(path) + "/up.png").c_str());
  cubeMap[3].load((std::string(path) + "/down.png").c_str());
  cubeMap[4].load((std::string(path) + "/right.png").c_str());
  cubeMap[5].load((std::string(path) + "/left.png").c_str());
}