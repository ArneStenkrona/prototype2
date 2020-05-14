#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(char const * assetDirectory)
    : m_modelManager((std::string(assetDirectory) + "models/").c_str()) {
      strcpy(m_assetDirectory, assetDirectory);
}

void AssetManager::loadCubeMap(char const * name, prt::array<Texture, 6>& cubeMap) {
  char path[256];
  strcpy(path, m_assetDirectory);
  strcat(path, "textures/skybox/");
  strcat(path, name);
  // std::string path = (m_assetDirectory + "textures/skybox/") + name;
  // strcpy(cubeMap[0].path, (path + "/front.png").c_str());
  // strcpy(cubeMap[1].path, (path + "/back.png").c_str());
  // strcpy(cubeMap[2].path, (path + "/up.png").c_str());
  // strcpy(cubeMap[3].path, (path + "/down.png").c_str());
  // strcpy(cubeMap[4].path, (path + "/right.png").c_str());
  // strcpy(cubeMap[5].path, (path + "/left.png").c_str());

  strcpy(cubeMap[0].path, path);
  strcat(cubeMap[0].path, "/front.png");
  strcpy(cubeMap[1].path, path);
  strcat(cubeMap[1].path, "/back.png");
  strcpy(cubeMap[2].path, path);
  strcat(cubeMap[2].path, "/up.png");
  strcpy(cubeMap[3].path, path);
  strcat(cubeMap[3].path, "/down.png");
  strcpy(cubeMap[4].path, path);
  strcat(cubeMap[4].path, "/right.png");
  strcpy(cubeMap[5].path, path);
  strcat(cubeMap[5].path, "/left.png");
  
  for (auto & tex : cubeMap) {
    tex.load();
  }
}