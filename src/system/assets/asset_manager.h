#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/game/level/level_map.h"

#include "src/system/assets/model_manager.h"

#include "src/container/array.h"

class AssetManager {
public:
    AssetManager(const char* assetDirectory);

    ModelManager& getModelManager() { return _modelManager; };

    inline void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                                prt::vector<Model>& models, 
                                prt::vector<uint32_t>& modelIndices,
                                prt::vector<Model>& colliderModels,
                                prt::vector<uint32_t>& colliderIDs) { _modelManager.loadSceneModels(modelIDs,
                                                                                                     models,
                                                                                                     modelIndices,
                                                                                                     colliderModels,
                                                                                                     colliderIDs);}
    void loadCubeMap(const char* name, prt::array<Texture, 6>& cubeMap);

    std::string getDirectory() const { return _assetDirectory; }

private:
    std::string _assetDirectory;
    ModelManager _modelManager;
};

#endif