#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/system/assets/model_manager.h"

#include "src/container/array.h"

class AssetManager {
public:
    AssetManager(char const * assetDirectory);

    ModelManager& getModelManager() { return m_modelManager; };

    void loadModels(char const * paths[], size_t count,
                    uint32_t * ids) { m_modelManager.loadModels(paths, count, ids); }

    // inline void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
    //                             prt::vector<Model>& models, 
    //                             prt::vector<uint32_t>& modelIndices) { _modelManager.loadSceneModels(modelIDs,
    //                                                                                                  models,
    //                                                                                                  modelIndices);}
    void loadCubeMap(char const * name, prt::array<Texture, 6>& cubeMap);

    std::string getDirectory() const { return m_assetDirectory; }

private:
    // std::string m_assetDirectory;
    char m_assetDirectory[256];
    ModelManager m_modelManager;
};

#endif