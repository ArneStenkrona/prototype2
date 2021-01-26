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

    // void loadModels(char const * paths[], size_t count,
    //                 ModelID * ids, bool animated) { m_modelManager.loadModels(paths, count, ids, animated); }

    void loadCubeMap(char const * name, prt::array<Texture, 6>& cubeMap);

    std::string getDirectory() const { return m_assetDirectory; }

private:
    char m_assetDirectory[256];
    TextureManager m_textureManager;
    ModelManager m_modelManager;
};

#endif