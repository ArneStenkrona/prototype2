#include "game_renderer.h"

GameRenderer::GameRenderer()
    : VulkanApplication() {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::createMaterialPipelines(prt::vector<Model> const & models) {
    size_t assetIndex = pushBackAssets(sizeof(SkyboxUBO));
    createSkyboxMaterialPipeline(assetIndex);

    // create pipelines for different shaders
    assetIndex = pushBackAssets(sizeof(StandardUBO));

    char vert[256] = RESOURCE_PATH;
    strcat(vert, "shaders/standard.vert.spv");
    for (auto & model : models) {
        for (auto & mesh : model.meshes) {
            auto const & material = model.materials[mesh.materialIndex];
            if (shaderToIndex.find(material.fragmentShader) == shaderToIndex.end()) {
                char frag[256] = RESOURCE_PATH;
                strcat(frag, "shaders/");
                strcat(frag, material.fragmentShader);
                strcat(frag, ".spv");
                // strcat(frag, "shaders/standard.frag.spv");
                createMeshMaterialPipeline(assetIndex, vert, frag);
                shaderToIndex[material.fragmentShader] = meshMaterialPipelineIndices.back();
            }
        }
    }
}

void GameRenderer::createSkyboxMaterialPipeline(size_t assetIndex) {
    skyboxPipelineIndex = materialPipelines.size();
    materialPipelines.push_back(MaterialPipeline{});
    MaterialPipeline& skyboxPipeline = materialPipelines.back();

    skyboxPipeline.assetsIndex = assetIndex;
    Assets const & ass = getAssets(assetIndex);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding cubeMapSamplerLayoutBinding = {};
    cubeMapSamplerLayoutBinding.descriptorCount = 1;
    cubeMapSamplerLayoutBinding.binding = 1;
    cubeMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    cubeMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubeMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

    skyboxPipeline.descriptorSetLayoutBindings.resize(2);
    skyboxPipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    skyboxPipeline.descriptorSetLayoutBindings[1] = cubeMapSamplerLayoutBinding;
    // Descriptor pools
    skyboxPipeline.descriptorPoolSizes.resize(2);
    skyboxPipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    skyboxPipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    skyboxPipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    skyboxPipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    // Descriptor sets
    skyboxPipeline.descriptorSets.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
        skyboxPipeline.descriptorBufferInfos[i].buffer = ass.uniformBufferData.uniformBuffers[i];
        skyboxPipeline.descriptorBufferInfos[i].offset = 0;
        skyboxPipeline.descriptorBufferInfos[i].range = ass.uniformBufferData.uboData.size();

        skyboxPipeline.descriptorImageInfos.resize(1);
        // skyboxPipeline.descriptorImageInfos[0].sampler = textureSampler;
        skyboxPipeline.descriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // skyboxPipeline.descriptorImageInfos[0].imageView = cubeMapImageView;

        skyboxPipeline.descriptorWrites[i].resize(2, VkWriteDescriptorSet{});
        
        skyboxPipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //skyboxPipeline.descriptorWrites[i][0].dstSet = skyboxPipeline.descriptorSets[i];
        skyboxPipeline.descriptorWrites[i][0].dstBinding = 0;
        skyboxPipeline.descriptorWrites[i][0].dstArrayElement = 0;
        skyboxPipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxPipeline.descriptorWrites[i][0].descriptorCount = 1;
        // skyboxPipeline.descriptorWrites[i][0].pBufferInfo = &skyboxPipeline.descriptorBufferInfos[i];

        skyboxPipeline.descriptorWrites[i][1] = {};
        skyboxPipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //skyboxPipeline.descriptorWrites[i][1].dstSet = skyboxPipeline.descriptorSets[i];
        skyboxPipeline.descriptorWrites[i][1].dstBinding = 1;
        skyboxPipeline.descriptorWrites[i][1].dstArrayElement = 0;
        skyboxPipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxPipeline.descriptorWrites[i][1].descriptorCount = 1;
        skyboxPipeline.descriptorWrites[i][1].pBufferInfo = 0;
        // skyboxPipeline.descriptorWrites[i][1].pImageInfo = skyboxPipeline.descriptorImageInfos.data();
    }

    // Vertex input
    skyboxPipeline.vertexInputBinding.binding = 0;
    skyboxPipeline.vertexInputBinding.stride = sizeof(glm::vec3);
    skyboxPipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    skyboxPipeline.vertexInputAttributes.resize(1);
    skyboxPipeline.vertexInputAttributes[0].binding = 0;
    skyboxPipeline.vertexInputAttributes[0].location = 0;
    skyboxPipeline.vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    skyboxPipeline.vertexInputAttributes[0].offset = 0;

    skyboxPipeline.vertexShader[0] = '\0';
    strcat(skyboxPipeline.vertexShader, RESOURCE_PATH);
    strcat(skyboxPipeline.vertexShader, "shaders/skybox.vert.spv");
    skyboxPipeline.fragmentShader[0] = '\0';
    strcat(skyboxPipeline.fragmentShader, RESOURCE_PATH);
    strcat(skyboxPipeline.fragmentShader, "shaders/skybox.frag.spv");
}
void GameRenderer::createMeshMaterialPipeline(size_t assetIndex, const char* vertexShader, const char* fragmentShader) {
    meshMaterialPipelineIndices.push_back(materialPipelines.size());
    materialPipelines.push_back(MaterialPipeline{});
    MaterialPipeline& modelPipeline = materialPipelines.back();

    modelPipeline.assetsIndex = assetIndex;
    Assets const & ass = getAssets(assetIndex);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.descriptorCount = NUMBER_SUPPORTED_TEXTURES;
    textureLayoutBinding.binding = 1;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;;
    textureLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    modelPipeline.descriptorSetLayoutBindings.resize(3);
    modelPipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    modelPipeline.descriptorSetLayoutBindings[1] = textureLayoutBinding;
    modelPipeline.descriptorSetLayoutBindings[2] = samplerLayoutBinding;

    // Descriptor pools
    modelPipeline.descriptorPoolSizes.resize(3);
    modelPipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelPipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    modelPipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    modelPipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_TEXTURES * swapChainImages.size());
    modelPipeline.descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    modelPipeline.descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // Descriptor sets
    modelPipeline.descriptorSets.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        modelPipeline.descriptorBufferInfos[i].buffer = ass.uniformBufferData.uniformBuffers[i];
        modelPipeline.descriptorBufferInfos[i].offset = 0;
        modelPipeline.descriptorBufferInfos[i].range = ass.uniformBufferData.uboData.size();
        
        modelPipeline.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);
        // for (size_t j = 0; j < modelPipeline.descriptorImageInfos.size(); j++) {
        //      modelPipeline.descriptorImageInfos[j].sampler = textureSampler;
        //      modelPipeline.descriptorImageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //      modelPipeline.descriptorImageInfos[j].imageView = modelPipeline.textureImages.imageViews[j];
        // }
        
        modelPipeline.descriptorWrites[i].resize(3, VkWriteDescriptorSet{});
        
        modelPipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //modelPipeline.descriptorWrites[i][0].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][0].dstBinding = 0;
        modelPipeline.descriptorWrites[i][0].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        modelPipeline.descriptorWrites[i][0].descriptorCount = 1;
        // modelPipeline.descriptorWrites[i][0].pBufferInfo = &modelPipeline.descriptorBufferInfos[i];

        modelPipeline.descriptorWrites[i][1] = {};
        modelPipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //modelPipeline.descriptorWrites[i][1].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][1].dstBinding = 1;
        modelPipeline.descriptorWrites[i][1].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        modelPipeline.descriptorWrites[i][1].descriptorCount = NUMBER_SUPPORTED_TEXTURES;
        modelPipeline.descriptorWrites[i][1].pBufferInfo = 0;
        // modelPipeline.descriptorWrites[i][1].pImageInfo = modelPipeline.descriptorImageInfos.data();

        // VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = textureSampler;

        modelPipeline.descriptorWrites[i][2] = {};
        modelPipeline.descriptorWrites[i][2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //modelPipeline.descriptorWrites[i][2].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][2].dstBinding = 2;
        modelPipeline.descriptorWrites[i][2].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        modelPipeline.descriptorWrites[i][2].descriptorCount = 1;
        modelPipeline.descriptorWrites[i][2].pBufferInfo = 0;
        modelPipeline.descriptorWrites[i][2].pImageInfo = &samplerInfo;
    }

    // Vertex input
    modelPipeline.vertexInputBinding = Vertex::getBindingDescription();
    auto attrib = Vertex::getAttributeDescriptions();
    modelPipeline.vertexInputAttributes.resize(attrib.size());
    size_t inIndx = 0;
    for (auto const & att : attrib) {
        modelPipeline.vertexInputAttributes[inIndx] = att;
        ++inIndx;
    }

    modelPipeline.vertexShader[0] = '\0';
    strcat(modelPipeline.vertexShader, vertexShader);
    modelPipeline.fragmentShader[0] = '\0';
    strcat(modelPipeline.fragmentShader, fragmentShader);
}

void GameRenderer::bindScene(Scene const & scene) {
    prt::vector<Model> models;
    prt::vector<uint32_t> modelIndices;
    scene.getModels(models, modelIndices);

    createMaterialPipelines(models);
    loadModels(models);

    prt::array<Texture, 6> skybox;
    scene.getSkybox(skybox);
    loadSkybox(skybox);

    createDrawCalls(models, modelIndices);

    recreateSwapChain();
}

void GameRenderer::update(prt::vector<glm::mat4> const & modelMatrices, 
                          glm::mat4 const & viewMatrix, 
                          glm::mat4 const & projectionMatrix, 
                          glm::vec3 const & viewPosition,
                          glm::mat4 const & skyProjectionMatrix,
                          DirLight  const & sun,
                          float time) {      
    updateUBOs(modelMatrices, 
               viewMatrix, 
               projectionMatrix, 
               viewPosition,
               skyProjectionMatrix,
               sun,
               time);
    VulkanApplication::update();   
}

void GameRenderer::updateUBOs(prt::vector<glm::mat4>  const & modelMatrices, 
                              glm::mat4 const & viewMatrix, 
                              glm::mat4 const & projectionMatrix, 
                              glm::vec3 const & viewPosition,
                              glm::mat4 const & skyProjectionMatrix,
                              DirLight  const & sun,
                              float time) {
    // skybox ubo
    // SkyboxUBO& skyboxUBO = *(new (getAssets(materialPipelines[skyboxPipelineIndex].assetsIndex).uniformBufferData.uboData.data()) SkyboxUBO{});
    auto skyboxUboData = getAssets(materialPipelines[skyboxPipelineIndex].assetsIndex).uniformBufferData.uboData.data();
    SkyboxUBO & skyboxUBO = *reinterpret_cast<SkyboxUBO*>(skyboxUboData);

    glm::mat4 skyboxViewMatrix = viewMatrix;
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix * glm::mat4(1.0f);
    skyboxUBO.projection = skyProjectionMatrix;
    skyboxUBO.projection[1][1] *= -1;

    // standard ubo                     
    // StandardUBO& standardUBO = *(new (getAssets(materialPipelines[meshMaterialPipelineIndices[0]].assetsIndex).uniformBufferData.uboData.data()) StandardUBO{});
    auto standardUboData = getAssets(materialPipelines[meshMaterialPipelineIndices[0]].assetsIndex).uniformBufferData.uboData.data();
    StandardUBO & standardUBO = *reinterpret_cast<StandardUBO*>(standardUboData);
    // model
    for (size_t i = 0; i < modelMatrices.size(); i++) {
        standardUBO.model.model[i] = modelMatrices[i];
        standardUBO.model.invTransposeModel[i] = glm::transpose(glm::inverse(modelMatrices[i]));
    }
    standardUBO.model.view = viewMatrix;
    standardUBO.model.proj = projectionMatrix;
    standardUBO.model.proj[1][1] *= -1;
    standardUBO.model.viewPosition = viewPosition;
    standardUBO.model.t = time;

    // lights
    standardUBO.lighting.sun = sun;
    standardUBO.lighting.ambientLight = 1.0f;
    standardUBO.lighting.noPointLights = 0;

}

void GameRenderer::loadModels(const prt::vector<Model>& models) {
    assert(!models.empty());

    createVertexBuffer(models);
    createIndexBuffer(models);

    size_t assetsIndex = materialPipelines[meshMaterialPipelineIndices[0]].assetsIndex;
    Assets & ass = getAssets(assetsIndex);

    size_t numTex = 0;
    ass.textureImages.images.resize(NUMBER_SUPPORTED_TEXTURES);
    ass.textureImages.imageViews.resize(NUMBER_SUPPORTED_TEXTURES);
    ass.textureImages.imageMemories.resize(NUMBER_SUPPORTED_TEXTURES);
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i].meshes.size(); j++) {
            auto const & material = models[i].materials[models[i].meshes[j].materialIndex];
            auto const & texture = models[i].textures[material.albedoIndex];
            createTextureImage(ass.textureImages.images[numTex], 
                               ass.textureImages.imageMemories[numTex], 
                               texture);
            createTextureImageView(ass.textureImages.imageViews[numTex], 
                                   ass.textureImages.images[numTex], 
                                   texture.mipLevels);
            numTex++;
        }
    }

    for (size_t i = numTex; i < ass.textureImages.images.size(); i++) {
        createTextureImage(ass.textureImages.images[i], 
                           ass.textureImages.imageMemories[i], 
                           models[0].textures.back());
        createTextureImageView(ass.textureImages.imageViews[i], 
                               ass.textureImages.images[i], 
                               models[0].textures.back().mipLevels);
    }
}

void GameRenderer::loadSkybox(const prt::array<Texture, 6>& skybox) {  
    createSkyboxBuffers();

    MaterialPipeline& skyboxPipeline = materialPipelines[skyboxPipelineIndex];
    Assets& ass = getAssets(skyboxPipeline.assetsIndex);

    ass.textureImages.images.resize(1);
    ass.textureImages.imageViews.resize(1);
    ass.textureImages.imageMemories.resize(1);
    createCubeMapImage(ass.textureImages.images[0], 
                       ass.textureImages.imageMemories[0], 
                       skybox);
    createCubeMapImageView(ass.textureImages.imageViews[0], 
                           ass.textureImages.images[0], 
                           skybox[0].mipLevels);
}

void GameRenderer::createDrawCalls(const prt::vector<Model>& models, 
                                         const prt::vector<uint32_t>& modelIndices) {
    // skybox
    {
        DrawCall drawCall;
        drawCall.firstIndex = 0;
        drawCall.indexCount = 36;
        materialPipelines[skyboxPipelineIndex].drawCalls.push_back(drawCall);
    }
    // models
    prt::vector<uint32_t> imgIdxOffsets = { 0 };
    prt::vector<uint32_t> indexOffsets = { 0 };
    imgIdxOffsets.resize(models.size());
    indexOffsets.resize(models.size());
    for (size_t i = 1; i < models.size(); i++) {
        imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1].meshes.size();
        indexOffsets[i] = indexOffsets[i-1] + models[i-1].indexBuffer.size();
    }

    for (size_t i = 0; i < modelIndices.size(); i++) {
        const Model& model = models[modelIndices[i]];
        for (auto const & mesh : model.meshes) {
            auto const & material = model.materials[mesh.materialIndex];
            DrawCall drawCall;
            // push constants
            new (&drawCall.pushConstants[0]) int32_t(i);
            new (&drawCall.pushConstants[4]) int32_t(imgIdxOffsets[modelIndices[i]] + material.albedoIndex);
            new (&drawCall.pushConstants[8]) int32_t(imgIdxOffsets[modelIndices[i]] + material.normalIndex);
            new (&drawCall.pushConstants[12]) int32_t(imgIdxOffsets[modelIndices[i]] + material.specularIndex);
            new (&drawCall.pushConstants[12]) glm::vec3(material.baseColor);
            new (&drawCall.pushConstants[28]) float(material.baseSpecularity);
            // geometry
            drawCall.firstIndex = indexOffsets[modelIndices[i]] + mesh.startIndex;
            drawCall.indexCount = mesh.numIndices;
            // pipeline index
            size_t index = shaderToIndex[material.fragmentShader];
            materialPipelines[index].drawCalls.push_back(drawCall);
        }
    }
}

void GameRenderer::createVertexBuffer(const prt::vector<Model>& models) {
    prt::vector<Vertex> allVertices;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i].vertexBuffer.size(); j++) {
            allVertices.push_back(models[i].vertexBuffer[j]);
        }
    }
    size_t assetsIndex = materialPipelines[meshMaterialPipelineIndices[0]].assetsIndex;
    VertexData& data = getAssets(assetsIndex).vertexData;
    createAndMapBuffer(allVertices.data(), sizeof(Vertex) * allVertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    
}

void GameRenderer::createIndexBuffer(const prt::vector<Model>& models) {
    prt::vector<uint32_t> allIndices;
    size_t vertexOffset = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i].indexBuffer.size(); j++) {
            allIndices.push_back(models[i].indexBuffer[j] + vertexOffset);
        }
        vertexOffset += models[i].vertexBuffer.size();
    }
    size_t assetsIndex = materialPipelines[meshMaterialPipelineIndices[0]].assetsIndex;
    VertexData& data = getAssets(assetsIndex).vertexData;
    createAndMapBuffer(allIndices.data(), sizeof(uint32_t) * allIndices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       data.indexBuffer, 
                       data.indexBufferMemory);
}

void GameRenderer::createSkyboxBuffers() {
    prt::vector<glm::vec3> vertices;

    vertices.resize(8);
    float w = 500.0f;
    vertices[7] = glm::vec3{-w, -w, -w};
    vertices[6] = glm::vec3{ w, -w, -w};
    vertices[5] = glm::vec3{-w,  w, -w};
    vertices[4] = glm::vec3{ w,  w, -w};

    vertices[3] = glm::vec3{-w, -w,  w};
    vertices[2] = glm::vec3{ w, -w,  w};
    vertices[1] = glm::vec3{-w,  w,  w};
    vertices[0] = glm::vec3{ w,  w,  w};

    size_t assetsIndex = materialPipelines[skyboxPipelineIndex].assetsIndex;
    VertexData& data = getAssets(assetsIndex).vertexData;
    createAndMapBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    

    prt::vector<uint32_t> indices = { 0, 2, 3,
                                      3, 1, 0,
                                      4, 5, 7,
                                      7, 6, 4,
                                      0, 1, 5,
                                      5, 4, 0,
                                      1, 3, 7,
                                      7, 5, 1,
                                      3, 2, 6,
                                      6, 7, 3,
                                      2, 0, 4,
                                      4, 6, 2 };
    
    createAndMapBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                    VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                    data.indexBuffer, 
                    data.indexBufferMemory);   
}
