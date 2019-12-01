#ifndef ASSET_MANAGER_H
#define ASSET_MANAGER_H

#include "src/system/assets/model_manager.h"

class AssetManager {
public:
    AssetManager(const char* assetDirectory);

    ModelManager& getModelManager() { return _modelManager; };

    void loadModels(prt::vector<Model>& models);
    /*void loadMeshes(const char* modelPath, prt::vector<Mesh>& meshes,
                                          prt::vector<Vertex>& vertexBuffer,
                                          prt::vector<uint32_t>& indexBuffer);

    void loadTextures(const char* texturePath, Texture& texture);*/

private:
    ModelManager _modelManager;
};

#endif