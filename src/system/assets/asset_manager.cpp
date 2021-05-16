#include "asset_manager.h"

#include <dirent.h>
#include <cstring>

AssetManager::AssetManager(char const * assetDirectory)
    : m_textureManager((std::string(assetDirectory) + "textures/").c_str()),
      m_modelManager((std::string(assetDirectory) + "models/").c_str(), m_textureManager) {
      strcpy(m_assetDirectory, assetDirectory);
}

void AssetManager::loadCubeMap(char const * name, prt::array<Texture, 6>& cubeMap) const {
    char path[256];
    strcpy(path, m_assetDirectory);
    strcat(path, "textures/skybox/");
    strcat(path, name);

    char * tok = path + strlen(path);
    strcpy(tok, "/front.png");
    cubeMap[0].load(path);
    strcpy(tok, "/back.png");
    cubeMap[1].load(path);
    strcpy(tok, "/up.png");
    cubeMap[2].load(path);
    strcpy(tok, "/down.png");
    cubeMap[3].load(path);
    strcpy(tok, "/right.png");
    cubeMap[4].load(path);
    strcpy(tok, "/left.png");
    cubeMap[5].load(path);
}