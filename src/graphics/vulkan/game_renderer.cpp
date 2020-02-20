#include "game_renderer.h"

GameRenderer::GameRenderer()
    : VulkanApplication() {
    createMaterialPipelines();
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::createMaterialPipelines() {
    createSkyboxMaterialPipeline();
    char vert[256] = RESOURCE_PATH;
    strcat(vert, "shaders/model.vert.spv");
    char frag[256] = RESOURCE_PATH;
    strcat(frag, "shaders/cel.frag.spv");
    createMeshMaterialPipeline(vert, frag);
}

void GameRenderer::createSkyboxMaterialPipeline() {
    skyboxPipelineIndex = materialPipelines.size();
    materialPipelines.push_back(MaterialPipeline{});
    MaterialPipeline& skyboxPipeline = materialPipelines.back();

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
    skyboxPipeline.uniformBuffers.resize(swapChainImages.size());
    skyboxPipeline.uniformBufferMemories.resize(swapChainImages.size());
    skyboxPipeline.descriptorSets.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
        // skyboxPipeline.descriptorBufferInfos[i].buffer = skyboxPipeline.uniformBuffers[i];
        skyboxPipeline.descriptorBufferInfos[i].offset = 0;
        skyboxPipeline.descriptorBufferInfos[i].range = sizeof(SkyboxUBO);

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
        skyboxPipeline.descriptorWrites[i][0].pBufferInfo = &skyboxPipeline.descriptorBufferInfos[i];

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

    // Resize ubo
    skyboxPipeline.uboData.resize(sizeof(SkyboxUBO));
}
void GameRenderer::createMeshMaterialPipeline(const char* vertexShader, const char* fragmentShader) {
    meshMaterialPipelineIndices.push_back(materialPipelines.size());
    materialPipelines.push_back(MaterialPipeline{});
    MaterialPipeline& modelPipeline = materialPipelines.back();

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;// | VK_SHADER_STAGE_FRAGMENT_BIT;

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
    modelPipeline.uniformBuffers.resize(swapChainImages.size());
    modelPipeline.uniformBufferMemories.resize(swapChainImages.size());
    modelPipeline.descriptorSets.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        // modelPipeline.descriptorBufferInfos[i].buffer = modelPipeline.uniformBuffers[i];
        modelPipeline.descriptorBufferInfos[i].offset = 0;
        modelPipeline.descriptorBufferInfos[i].range = sizeof(ModelUBO);
        
        modelPipeline.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);
        for (size_t j = 0; j < modelPipeline.descriptorImageInfos.size(); j++) {
            // modelPipeline.descriptorImageInfos[j].sampler = textureSampler;
            modelPipeline.descriptorImageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            // modelPipeline.descriptorImageInfos[j].imageView = modelPipeline.textureImages.imageViews[j];
        }
        
        modelPipeline.descriptorWrites[i].resize(3, VkWriteDescriptorSet{});
        
        modelPipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        //modelPipeline.descriptorWrites[i][0].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][0].dstBinding = 0;
        modelPipeline.descriptorWrites[i][0].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        modelPipeline.descriptorWrites[i][0].descriptorCount = 1;
        modelPipeline.descriptorWrites[i][0].pBufferInfo = &modelPipeline.descriptorBufferInfos[i];

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
    modelPipeline.vertexInputAttributes.resize(3);
    modelPipeline.vertexInputAttributes[0] = attrib[0];
    modelPipeline.vertexInputAttributes[1] = attrib[1];
    modelPipeline.vertexInputAttributes[2] = attrib[2];

    modelPipeline.vertexShader[0] = '\0';
    strcat(modelPipeline.vertexShader, vertexShader);
    modelPipeline.fragmentShader[0] = '\0';
    strcat(modelPipeline.fragmentShader, fragmentShader);

    // Resize ubo
    modelPipeline.uboData.resize(sizeof(ModelUBO));
}

void GameRenderer::bindScene(Scene const & scene) {
    prt::vector<Model> models;
    prt::vector<uint32_t> modelIndices;
    scene.getModels(models, modelIndices);
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
                          float time) {      
    updateUBOs(modelMatrices, 
               viewMatrix, 
               projectionMatrix, 
               viewPosition,
               skyProjectionMatrix,
               time);
    VulkanApplication::update();   
}

void GameRenderer::updateUBOs(prt::vector<glm::mat4>  const & modelMatrices, 
                              glm::mat4 const & viewMatrix, 
                              glm::mat4 const & projectionMatrix, 
                              glm::vec3 const & viewPosition,
                              glm::mat4 const & skyProjectionMatrix,
                              float time) {
    // skybox
    SkyboxUBO& skyboxUBO = *(new (materialPipelines[skyboxPipelineIndex].uboData.data()) SkyboxUBO{});

    glm::mat4 skyboxViewMatrix = viewMatrix;
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix * glm::mat4(1.0f);
    skyboxUBO.projection = skyProjectionMatrix;
    skyboxUBO.projection[1][1] *= -1;

    // model                         
    ModelUBO& modelUBO = *(new (materialPipelines[meshMaterialPipelineIndices[0]].uboData.data()) ModelUBO{});
    for (size_t i = 0; i < modelMatrices.size(); i++) {
        modelUBO.model[i] = modelMatrices[i];
        modelUBO.invTransposeModel[i] = glm::transpose(glm::inverse(modelMatrices[i]));
    }
    modelUBO.view = viewMatrix;
    modelUBO.proj = projectionMatrix;
    modelUBO.proj[1][1] *= -1;
    modelUBO.viewPosition = viewPosition;
    modelUBO.t = time;
}

void GameRenderer::loadModels(const prt::vector<Model>& models) {
    assert(!models.empty());

    createVertexBuffer(models);
    createIndexBuffer(models);

    MaterialPipeline& modelPipeline = materialPipelines[meshMaterialPipelineIndices[0]];
    size_t numTex = 0;
    modelPipeline.textureImages.images.resize(NUMBER_SUPPORTED_TEXTURES);
    modelPipeline.textureImages.imageViews.resize(NUMBER_SUPPORTED_TEXTURES);
    modelPipeline.textureImages.imageMemories.resize(NUMBER_SUPPORTED_TEXTURES);
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._meshes.size(); j++) {
            createTextureImage(modelPipeline.textureImages.images[numTex], 
                               modelPipeline.textureImages.imageMemories[numTex], 
                               models[i]._meshes[j]._texture);
            createTextureImageView(modelPipeline.textureImages.imageViews[numTex], 
                                   modelPipeline.textureImages.images[numTex], 
                                   models[i]._meshes[j]._texture.mipLevels);
            numTex++;
        }
    }

    for (size_t i = numTex; i < modelPipeline.textureImages.images.size(); i++) {
        createTextureImage(modelPipeline.textureImages.images[i], 
                           modelPipeline.textureImages.imageMemories[i], 
                           models[0]._meshes.back()._texture);
        createTextureImageView(modelPipeline.textureImages.imageViews[i], 
                               modelPipeline.textureImages.images[i], 
                               models[0]._meshes.back()._texture.mipLevels);
    }
}

void GameRenderer::loadSkybox(const prt::array<Texture, 6>& skybox) {   
    createSkyboxBuffers();
    MaterialPipeline& skyboxPipeline = materialPipelines[skyboxPipelineIndex];
    skyboxPipeline.textureImages.images.resize(1);
    skyboxPipeline.textureImages.imageViews.resize(1);
    skyboxPipeline.textureImages.imageMemories.resize(1);
    createCubeMapImage(skyboxPipeline.textureImages.images[0], 
                       skyboxPipeline.textureImages.imageMemories[0], 
                       skybox);
    createCubeMapImageView(skyboxPipeline.textureImages.imageViews[0], 
                           skyboxPipeline.textureImages.images[0], 
                           skybox[0].mipLevels);
}

void GameRenderer::createDrawCalls(const prt::vector<Model>& models, 
                                         const prt::vector<uint32_t>& modelIndices) {
    // skybox
    {
        DrawCall drawCall;
        drawCall.pushConstants[0] = 0;
        drawCall.pushConstants[1] = 0;
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
        imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1]._meshes.size();
        indexOffsets[i] = indexOffsets[i-1] + models[i-1]._indexBuffer.size();
    }

    for (size_t i = 0; i < modelIndices.size(); i++) {
        const Model& model = models[modelIndices[i]];
        for (size_t j = 0; j < model._meshes.size(); j++) {
            DrawCall drawCall;
            drawCall.pushConstants[0] = i;
            drawCall.pushConstants[1] = imgIdxOffsets[modelIndices[i]] + j;
            drawCall.firstIndex = indexOffsets[modelIndices[i]] + model._meshes[j].startIndex;
            drawCall.indexCount = model._meshes[j].numIndices;
            materialPipelines[meshMaterialPipelineIndices[0]].drawCalls.push_back(drawCall);
        }
    }
}

void GameRenderer::createVertexBuffer(const prt::vector<Model>& models) {
    prt::vector<Vertex> allVertices;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._vertexBuffer.size(); j++) {
            allVertices.push_back(models[i]._vertexBuffer[j]);
        }
    }
    createAndMapBuffer(allVertices.data(), sizeof(Vertex) * allVertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       materialPipelines[meshMaterialPipelineIndices[0]].vertexData.vertexBuffer, 
                       materialPipelines[meshMaterialPipelineIndices[0]].vertexData.vertexBufferMemory);    
}

void GameRenderer::createIndexBuffer(const prt::vector<Model>& models) {
    prt::vector<uint32_t> allIndices;
    size_t vertexOffset = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._indexBuffer.size(); j++) {
            allIndices.push_back(models[i]._indexBuffer[j] + vertexOffset);
        }
        vertexOffset += models[i]._vertexBuffer.size();
    }

    createAndMapBuffer(allIndices.data(), sizeof(uint32_t) * allIndices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       materialPipelines[meshMaterialPipelineIndices[0]].vertexData.indexBuffer, 
                       materialPipelines[meshMaterialPipelineIndices[0]].vertexData.indexBufferMemory);
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

    createAndMapBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       materialPipelines[skyboxPipelineIndex].vertexData.vertexBuffer, 
                       materialPipelines[skyboxPipelineIndex].vertexData.vertexBufferMemory);    

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
                    materialPipelines[skyboxPipelineIndex].vertexData.indexBuffer, 
                    materialPipelines[skyboxPipelineIndex].vertexData.indexBufferMemory);   
}