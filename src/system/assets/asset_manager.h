#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/system/assets/model_manager.h"
#include "src/system/assets/texture_manager.h"

#include "src/container/array.h"

class AssetManager {
public:
    AssetManager(char const * assetDirectory);

    ModelManager& getModelManager() { return m_modelManager; };
    TextureManager& getTextureManager() { return m_textureManager; };

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids, bool animated = false) { m_modelManager.loadModels(paths, count, ids, animated, m_textureManager); }

    void loadCubeMap(char const * name, prt::array<Texture, 6>& cubeMap);

    std::string getDirectory() const { return m_assetDirectory; }

private:
    // std::string m_assetDirectory;
    char m_assetDirectory[256];
    ModelManager m_modelManager;
    TextureManager m_textureManager;
};

#endif