#ifndef TEXTURE_MANAGER_H
#define TEXTURE_MANAGER_H

#include "src/graphics/geometry/texture.h"

#include "src/container/hash_map.h"

class TextureManager {
public:
    TextureManager(const char* directory);

    inline void getTextures(Texture const * & textures, 
                            size_t & nTextures) const { textures = m_loadedTextures.data();
                                                        nTextures = m_loadedTextures.size(); }

    Texture const & getTexture(uint32_t textureID) const { return m_loadedTextures[textureID]; }

    uint32_t loadTexture(char const * texturePath, bool fullPath = false);

private:
    prt::hash_map<std::string, uint32_t> m_pathToTextureID;
    char m_textureDirectory[256];
    prt::vector<Texture> m_loadedTextures;
    ;
};

#endif
