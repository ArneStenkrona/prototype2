#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/system/assets/model_manager.h"

class AssetManager {
public:
    AssetManager(const char* assetDirectory);

    ModelManager& getModelManager() { return _modelManager; };

    inline void loadModels(prt::vector<Model>& models) { _modelManager.loadModels(models); }

    static constexpr const char* persistentStorageString = "P:";
    static constexpr const char* NonPersistentStorageString = "N:";

private:
    ModelManager _modelManager;
};

#endif