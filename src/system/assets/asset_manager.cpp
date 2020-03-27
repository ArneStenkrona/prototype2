#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(const char* assetDirectory)
    : _assetDirectory(assetDirectory),
      _modelManager((_assetDirectory + "models/").c_str()) {
}

void AssetManager::loadCubeMap(const char* name, prt::array<Texture, 6>& cubeMap) {
  std::string path = (_assetDirectory + "textures/skybox/") + name;
  strcpy(cubeMap[0].path, (path + "/front.png").c_str());
  strcpy(cubeMap[1].path, (path + "/back.png").c_str());
  strcpy(cubeMap[2].path, (path + "/up.png").c_str());
  strcpy(cubeMap[3].path, (path + "/down.png").c_str());
  strcpy(cubeMap[4].path, (path + "/right.png").c_str());
  strcpy(cubeMap[5].path, (path + "/left.png").c_str());
  for (auto & tex : cubeMap) {
    tex.load();
  }
}