#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/game/level/level_map.h"

#include "src/system/assets/model_manager.h"

class AssetManager {
public:
    AssetManager(const char* assetDirectory);

    ModelManager& getModelManager() { return _modelManager; };

    inline void loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                                prt::vector<Model>& models, 
                                prt::vector<uint32_t>& modelIndices) { _modelManager.loadSceneModels(modelIDs,
                                                                                                     models,
                                                                                                     modelIndices);}

    static constexpr const char* persistentStorageString = "P:";
    static constexpr const char* NonPersistentStorageString = "N:";

    std::string getDirectory() const { return _assetDirectory; }

private:
    std::string _assetDirectory;
    ModelManager _modelManager;
};

#endif