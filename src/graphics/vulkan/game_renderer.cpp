#include "game_renderer.h"

void GameRenderer::bindScene(Scene const & scene) {
    prt::vector<Model> models;
    prt::vector<uint32_t> modelIndices;
    scene.getModels(models, modelIndices);
    loadModels(models);

    prt::array<Texture, 6> skybox;
    scene.getSkybox(skybox);
    loadSkybox(skybox);
    createRenderJobs(models, modelIndices);
    recreateSwapChain();
}

void GameRenderer::loadModels(const prt::vector<Model>& models) {
    assert(!models.empty());
    for (size_t i = 0; i < pushConstants.size(); i++) {
        pushConstants[i] = i;
    }
    createVertexBuffer(models);
    createIndexBuffer(models);

    createSkyboxBuffers();

    size_t numTex = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._meshes.size(); j++) {
            createTextureImage(textureImage[numTex], textureImageMemory[numTex], models[i]._meshes[j]._texture);
            createTextureImageView(textureImageView[numTex], textureImage[numTex], models[i]._meshes[j]._texture.mipLevels);
            numTex++;
        }
    }

    for (size_t i = numTex; i < NUMBER_SUPPORTED_TEXTURES; i++) {
        createTextureImage(textureImage[i], textureImageMemory[i], models[0]._meshes.back()._texture);
        createTextureImageView(textureImageView[i], textureImage[i], models[0]._meshes.back()._texture.mipLevels);
    }
}

void GameRenderer::loadSkybox(const prt::array<Texture, 6>& skybox) {    
    createCubeMapImage(cubeMapImage, cubeMapImageMemory, skybox);
    createCubeMapImageView(cubeMapImageView, cubeMapImage, skybox[0].mipLevels);
}

void GameRenderer::createRenderJobs(const prt::vector<Model>& models, 
                                         const prt::vector<uint32_t>& modelIndices) {
    prt::vector<uint32_t> imgIdxOffsets = { 0 };
    prt::vector<uint32_t> indexOffsets = { 0 };
    imgIdxOffsets.resize(models.size());
    indexOffsets.resize(models.size());
    for (size_t i = 1; i < models.size(); i++) {
        imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1]._meshes.size();
        indexOffsets[i] = indexOffsets[i-1] + models[i-1]._indexBuffer.size();
    }

    for (size_t i = 0; i < modelIndices.size(); i++) {
        const Model& model = models[modelIndices[i]];
        for (size_t j = 0; j < model._meshes.size(); j++) {
            RenderJob renderJob;
            renderJob._modelMatrixIdx = i;
            renderJob._imgIdx = imgIdxOffsets[modelIndices[i]] + j;
            renderJob._firstIndex = indexOffsets[modelIndices[i]] + model._meshes[j].startIndex;
            renderJob._indexCount = model._meshes[j].numIndices;
            _renderJobs.push_back(renderJob);
        }
    }
}