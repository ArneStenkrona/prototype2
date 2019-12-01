#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/system/assets/model_manager.h"

#include "src/graphics/geometry/model.h"

class AssetManager {
public:
    AssetManager(const char* assetDirectory);

    ModelManager& getModelManager() { return _modelManager; };

    void loadModels(prt::vector<Model>& models);
    void loadMeshes(const char* modelPath, prt::vector<Mesh>& meshes,
                                          prt::vector<Vertex>& vertexBuffer,
                                          prt::vector<uint32_t>& indexBuffer);

    void loadTextures(const char* texturePath, Texture& texture);

    /*enum AssetTypes {
        MODEL,
        TEXTURE,
        TOTAL_ASSET_TYPES
    };*/
    /**
     * PERSISTENT data is located in secondary i.e in file system
     * NON_PERSISTENT data is created at runtime
     */
    /*enum StorageType{
        PERSISENT,
        NON_PERSISTENT,
        TOTAL_STORAGE_TYPES
    };*/
private:
    ModelManager _modelManager;
};

#endif