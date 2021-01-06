#include "texture_manager.h"

#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>

#include <string>   

#include <fstream>

TextureManager::TextureManager(const char* directory) {
    strcpy(m_textureDirectory, directory);
}

uint32_t TextureManager::loadTexture(char const * texturePath, bool fullPath) {    
    uint32_t id = 0;

    char path[256] = {};    
    if (!fullPath) {
        strcpy(path, m_textureDirectory);
    }
    strcat(path, texturePath);

    if (m_pathToTextureID.find(path) == m_pathToTextureID.end()) {
        id = m_loadedTextures.size();
        m_pathToTextureID.insert(path, id);
        m_loadedTextures.push_back({});
        Texture & texture = m_loadedTextures.back();
        texture.load(path);
    } else {
        id = m_pathToTextureID.find(path)->value();
    }

    return id;
}
