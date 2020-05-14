#include "game_renderer.h"

GameRenderer::GameRenderer(unsigned int width, unsigned int height)
    : VulkanApplication(width, height) {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::createGraphicsPipelines(size_t skyboxAssetIndex, size_t skyboxUboIndex, 
                                           size_t standardAssetIndex, size_t standardUboIndex, 
                                           size_t shadowmapAssetIndex, size_t shadowmapUboIndex) {
    createSkyboxGraphicsPipeline(skyboxAssetIndex, skyboxUboIndex);

    char vert[256] = RESOURCE_PATH;
    char frag[256] = RESOURCE_PATH;
    strcat(vert, "shaders/standard.vert.spv");
    strcat(frag, "shaders/standard.frag.spv");
    createStandardGraphicsPipeline(standardAssetIndex, standardUboIndex, vert, frag);

    vert[0] = '\0';
    frag[0] = '\0';
    strcpy(vert, RESOURCE_PATH);
    strcpy(frag, RESOURCE_PATH);
    strcat(vert, "shaders/shadow_map.vert.spv");
    strcat(frag, "shaders/shadow_map.frag.spv");

    createShadowmapGraphicsPipeline(shadowmapAssetIndex, shadowmapUboIndex,
                                    vert, frag);
}

void GameRenderer::createSkyboxGraphicsPipeline(size_t assetIndex, size_t uboIndex) {
    skyboxPipelineIndex = graphicsPipelines.scene.size();
    graphicsPipelines.scene.push_back(GraphicsPipeline{});
    GraphicsPipeline& skyboxPipeline = graphicsPipelines.scene.back();

    skyboxPipeline.assetsIndex = assetIndex;
    skyboxPipeline.uboIndex = uboIndex;
    Assets const & asset = getAssets(assetIndex);
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

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
    skyboxPipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
        skyboxPipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        skyboxPipeline.descriptorBufferInfos[i].offset = 0;
        skyboxPipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();

        // skyboxPipeline.descriptorImageInfos.resize(1);
        // skyboxPipeline.descriptorImageInfos[0].sampler = textureSampler;
        // skyboxPipeline.descriptorImageInfos[0].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        // skyboxPipeline.descriptorImageInfos[0].imageView = asset.textureImages.imageViews[0];

        skyboxPipeline.descriptorWrites[i].resize(2, VkWriteDescriptorSet{});
        
        skyboxPipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // skyboxPipeline.descriptorWrites[i][0].dstSet = skyboxPipeline.descriptorSets[i];
        skyboxPipeline.descriptorWrites[i][0].dstBinding = 0;
        skyboxPipeline.descriptorWrites[i][0].dstArrayElement = 0;
        skyboxPipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        skyboxPipeline.descriptorWrites[i][0].descriptorCount = 1;
        // skyboxPipeline.descriptorWrites[i][0].pBufferInfo = &skyboxPipeline.descriptorBufferInfos[i];

        skyboxPipeline.descriptorWrites[i][1] = {};
        skyboxPipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // skyboxPipeline.descriptorWrites[i][1].dstSet = skyboxPipeline.descriptorSets[i];
        skyboxPipeline.descriptorWrites[i][1].dstBinding = 1;
        skyboxPipeline.descriptorWrites[i][1].dstArrayElement = 0;
        skyboxPipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        skyboxPipeline.descriptorWrites[i][1].descriptorCount = 1;
        skyboxPipeline.descriptorWrites[i][1].pBufferInfo = 0;
        skyboxPipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();
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

    auto & shaderStages = skyboxPipeline.shaderStages;
    shaderStages.resize(2);

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, RESOURCE_PATH);
    strcat(shaderStages[0].shader, "shaders/skybox.vert.spv");

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName[0] = '\0';
    strcat(shaderStages[1].pName, "main");
    shaderStages[1].shader[0] = '\0';
    strcat(shaderStages[1].shader, RESOURCE_PATH);
    strcat(shaderStages[1].shader, "shaders/skybox.frag.spv");

    skyboxPipeline.extent = swapChainExtent;
    skyboxPipeline.renderpass = scenePass;
    skyboxPipeline.useColorAttachment = true;
    skyboxPipeline.enableDepthBias = false;
}
void GameRenderer::createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                                  const char* vertexShader, const char* fragmentShader) {
    standardPipelineIndex = graphicsPipelines.scene.size();
    graphicsPipelines.scene.push_back(GraphicsPipeline{});
    GraphicsPipeline& modelPipeline = graphicsPipelines.scene.back();

    modelPipeline.assetsIndex = assetIndex;
    modelPipeline.uboIndex = uboIndex;
    Assets const & asset = getAssets(assetIndex);
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

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

    VkDescriptorSetLayoutBinding shadowmapLayoutBinding = {};
    shadowmapLayoutBinding.descriptorCount = 1;
    shadowmapLayoutBinding.binding = 3;
    shadowmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;;
    shadowmapLayoutBinding.pImmutableSamplers = nullptr;

    modelPipeline.descriptorSetLayoutBindings.resize(4);
    modelPipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    modelPipeline.descriptorSetLayoutBindings[1] = textureLayoutBinding;
    modelPipeline.descriptorSetLayoutBindings[2] = samplerLayoutBinding;
    modelPipeline.descriptorSetLayoutBindings[3] = shadowmapLayoutBinding;

    // Descriptor pools
    modelPipeline.descriptorPoolSizes.resize(4);
    modelPipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    modelPipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    modelPipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    modelPipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_TEXTURES * swapChainImages.size());
    modelPipeline.descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    modelPipeline.descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    modelPipeline.descriptorPoolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    modelPipeline.descriptorPoolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // Descriptor sets
    modelPipeline.descriptorSets.resize(swapChainImages.size());
    modelPipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        modelPipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        modelPipeline.descriptorBufferInfos[i].offset = 0;
        modelPipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();
        
        // modelPipeline.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);
        // for (size_t j = 0; j < modelPipeline.descriptorImageInfos.size(); j++) {
        //      modelPipeline.descriptorImageInfos[j].sampler = textureSampler;
        //      modelPipeline.descriptorImageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        //      modelPipeline.descriptorImageInfos[j].imageView = asset.textureImages.imageViews[j];
        // }
        
        modelPipeline.descriptorWrites[i].resize(4, VkWriteDescriptorSet{});
        
        modelPipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // modelPipeline.descriptorWrites[i][0].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][0].dstBinding = 0;
        modelPipeline.descriptorWrites[i][0].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        modelPipeline.descriptorWrites[i][0].descriptorCount = 1;
        // modelPipeline.descriptorWrites[i][0].pBufferInfo = &modelPipeline.descriptorBufferInfos[i];

        modelPipeline.descriptorWrites[i][1] = {};
        modelPipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // modelPipeline.descriptorWrites[i][1].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][1].dstBinding = 1;
        modelPipeline.descriptorWrites[i][1].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        modelPipeline.descriptorWrites[i][1].descriptorCount = NUMBER_SUPPORTED_TEXTURES;
        modelPipeline.descriptorWrites[i][1].pBufferInfo = 0;
        modelPipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();

        // VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = textureSampler;

        modelPipeline.descriptorWrites[i][2] = {};
        modelPipeline.descriptorWrites[i][2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // modelPipeline.descriptorWrites[i][2].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][2].dstBinding = 2;
        modelPipeline.descriptorWrites[i][2].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        modelPipeline.descriptorWrites[i][2].descriptorCount = 1;
        modelPipeline.descriptorWrites[i][2].pBufferInfo = 0;
        modelPipeline.descriptorWrites[i][2].pImageInfo = &samplerInfo;

        modelPipeline.descriptorWrites[i][3] = {};
        modelPipeline.descriptorWrites[i][3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // modelPipeline.descriptorWrites[i][3].dstSet = modelPipeline.descriptorSets[i];
        modelPipeline.descriptorWrites[i][3].dstBinding = 3;
        modelPipeline.descriptorWrites[i][3].dstArrayElement = 0;
        modelPipeline.descriptorWrites[i][3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        modelPipeline.descriptorWrites[i][3].descriptorCount = 1;
        modelPipeline.descriptorWrites[i][3].pBufferInfo = 0;
        modelPipeline.descriptorWrites[i][3].pImageInfo = &offscreenPass.descriptors[i];
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
    auto & shaderStages = modelPipeline.shaderStages;
    shaderStages.resize(2);

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, vertexShader);

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName[0] = '\0';
    strcat(shaderStages[1].pName, "main");
    shaderStages[1].shader[0] = '\0';
    strcat(shaderStages[1].shader, fragmentShader);

    modelPipeline.extent = swapChainExtent;
    modelPipeline.renderpass = scenePass;
    modelPipeline.useColorAttachment = true;
    modelPipeline.enableDepthBias = false;
}

void GameRenderer::createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                                   const char* vertexShader, const char* /*fragmentShader*/) {
    shadowmapPipelineIndex = graphicsPipelines.offscreen.size();
    graphicsPipelines.offscreen.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.offscreen.back();

    pipeline.assetsIndex = assetIndex;
    pipeline.uboIndex = uboIndex;
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);
    
    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    pipeline.descriptorSetLayoutBindings.resize(1);
    pipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;

    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(1);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        pipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        pipeline.descriptorBufferInfos[i].offset = 0;
        pipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();
        
        pipeline.descriptorWrites[i].resize(1, VkWriteDescriptorSet{});
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].dstArrayElement = 0;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
    }

    // Vertex input
    pipeline.vertexInputBinding = Vertex::getBindingDescription();
    auto attrib = Vertex::getAttributeDescriptions();
    pipeline.vertexInputAttributes.resize(attrib.size());
    size_t inIndx = 0;
    for (auto const & att : attrib) {
        pipeline.vertexInputAttributes[inIndx] = att;
        ++inIndx;
    }
    auto & shaderStages = pipeline.shaderStages;
    shaderStages.resize(1);

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, vertexShader);

    // shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    // shaderStages[1].pName[0] = '\0';
    // strcat(shaderStages[1].pName, "main");
    // shaderStages[1].shader[0] = '\0';
    // strcat(shaderStages[1].shader, fragmentShader);

    pipeline.extent = offscreenPass.extent;
    pipeline.renderpass = offscreenPass.renderPass;
    pipeline.useColorAttachment = false;
    pipeline.enableDepthBias = true;
}

void GameRenderer::bindScene(Scene const & scene) {
    // prt::vector<Model> models;
    prt::vector<uint32_t> modelIDs;
    // scene.getModels(models, modelIndices);
    Model const * models;
    size_t nModels;
    // uint32_t const * modelIDs; 
    // size_t nModelIDs;
    scene.getModels(models, nModels, modelIDs);

    size_t skyboxAssetIndex = pushBackAssets();
    size_t skyboxUboIndex = pushBackUniformBufferData(sizeof(SkyboxUBO));
    size_t standardAssetIndex = pushBackAssets();
    size_t standardUboIndex = pushBackUniformBufferData(sizeof(StandardUBO));
    size_t shadowMapUboIndex = pushBackUniformBufferData(sizeof(ShadowMapUBO));

    reprepareSwapChain();

    prt::array<Texture, 6> skybox;
    scene.getSkybox(skybox);
    loadCubeMap(skybox, skyboxAssetIndex);

    loadModels(models, nModels, standardAssetIndex);

    createGraphicsPipelines(skyboxAssetIndex, skyboxUboIndex,
                            standardAssetIndex, standardUboIndex,
                            standardAssetIndex, shadowMapUboIndex);


    // createDrawCalls(models, modelIndices);
    createDrawCalls(models, nModels, modelIDs.data(), modelIDs.size());

    completeSwapChain();
}

void GameRenderer::update(prt::vector<glm::mat4> const & modelMatrices, 
                          Camera const & camera,
                          DirLight  const & sun,
                          float time) {      
    updateUBOs(modelMatrices, 
               camera,
               sun,
               time);
    VulkanApplication::update();   
}

void GameRenderer::updateUBOs(prt::vector<glm::mat4>  const & modelMatrices, 
                              Camera const & camera,
                              DirLight  const & sun,
                              float time) {
    glm::mat4 viewMatrix = camera.getViewMatrix();
    int w,h = 0;
    getWindowSize(w, h);
    glm::mat4 projectionMatrix = camera.getProjectionMatrix(float(w), float(h), nearPlane, farPlane);
    glm::mat4 skyProjectionMatrix = camera.getProjectionMatrix(float(w), float(h), nearPlane, 1000.0f);
    glm::vec3 viewPosition = camera.getPosition();

    // skybox ubo
    auto skyboxUboData = getUniformBufferData(graphicsPipelines.scene[skyboxPipelineIndex].uboIndex).uboData.data();
    SkyboxUBO & skyboxUBO = *reinterpret_cast<SkyboxUBO*>(skyboxUboData);

    glm::mat4 skyboxViewMatrix = viewMatrix;
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix * glm::mat4(1.0f);
    skyboxUBO.projection = skyProjectionMatrix;
    skyboxUBO.projection[1][1] *= -1;

    prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> cascadeSpace;
    prt::array<float, NUMBER_SHADOWMAP_CASCADES> splitDepths;
    
    updateCascades(projectionMatrix, viewMatrix, sun.direction, cascadeSpace, splitDepths);

    // standard ubo                     
    auto standardUboData = getUniformBufferData(graphicsPipelines.scene[standardPipelineIndex].uboIndex).uboData.data();
    StandardUBO & standardUBO = *reinterpret_cast<StandardUBO*>(standardUboData);
    // model
    for (size_t i = 0; i < modelMatrices.size(); ++i) {
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
    standardUBO.lighting.ambientLight = 0.2f;
    standardUBO.lighting.noPointLights = 0;

    for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
        standardUBO.lighting.cascadeSpace[i] = cascadeSpace[i];
        standardUBO.lighting.splitDepths[i/4][i%4] = splitDepths[i];
    }
    // shadow map ubo
    auto shadowUboData = getUniformBufferData(graphicsPipelines.offscreen[shadowmapPipelineIndex].uboIndex).uboData.data();
    ShadowMapUBO & shadowUBO = *reinterpret_cast<ShadowMapUBO*>(shadowUboData);
    // model
    std::memcpy(&shadowUBO.model[0], standardUBO.model.model, sizeof(standardUBO.model.model[0]) * NUMBER_SUPPORTED_MODEL_MATRICES);
     // depth view and projection;
    for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
        shadowUBO.depthVP[i] = cascadeSpace[i];
    }
}

void GameRenderer::updateCascades(glm::mat4 const & projectionMatrix,
                                  glm::mat4 const & viewMatrix,
                                  glm::vec3 const & lightDir,
                                  prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> & cascadeSpace,
                                  prt::array<float, NUMBER_SHADOWMAP_CASCADES> & splitDepths) {
    float cascadeSplits[NUMBER_SHADOWMAP_CASCADES];

    float clipRange = farPlane - nearPlane;

    float minZ = nearPlane;
    float maxZ = farPlane;

    float ratio = maxZ / minZ;
    for (unsigned int i = 0;  i < NUMBER_SHADOWMAP_CASCADES; ++i) {
        float p = (i + 1) / static_cast<float>(NUMBER_SHADOWMAP_CASCADES);
        float log = minZ * glm::pow(ratio, p);
        float uniform = minZ + clipRange * p;
        float d = cascadeSplitLambda * (log - uniform) + uniform;
        cascadeSplits[i] = (d - nearPlane) / clipRange;
    }

    float lastSplitDist = 0.0f;
    for (unsigned int i = 0; i < NUMBER_SHADOWMAP_CASCADES; ++i) {
        float splitDist = cascadeSplits[i];
        
        glm::vec3 frustumCorners[8] = {
            // near face
            glm::vec3(-1.0f,  1.0f, -1.0f),
            glm::vec3( 1.0f,  1.0f, -1.0f),
            glm::vec3( 1.0f, -1.0f, -1.0f),
            glm::vec3(-1.0f, -1.0f, -1.0f),

            // far face
            glm::vec3(-1.0f,  1.0f, 1.0f),
            glm::vec3( 1.0f,  1.0f, 1.0f),
            glm::vec3( 1.0f, -1.0f, 1.0f),
            glm::vec3(-1.0f, -1.0f, 1.0f)
        };

        glm::mat4 const camInv = glm::inverse(projectionMatrix * viewMatrix);
        for (unsigned int j = 0; j < 8; ++j) {
            glm::vec4 invCorner = camInv * glm::vec4(frustumCorners[j], 1.0f);
            frustumCorners[j] = invCorner / invCorner.w;
        }

        for (unsigned int i = 0; i < 4; ++i) {
            glm::vec3 dist = frustumCorners[i + 4] - frustumCorners[i];
            frustumCorners[i + 4] = frustumCorners[i] + (dist * splitDist);
            frustumCorners[i] = frustumCorners[i] + (dist * lastSplitDist);
        }

        // Get frustum center
        glm::vec3 frustumCenter{ 0.0f, 0.0f, 0.0f };
        for (unsigned int j = 0; j < 8; ++j) {
            frustumCenter += frustumCorners[j];
        }
        frustumCenter /= 8.0f;

        float radius = 0.0f;
        for (size_t j = 0; j < 8; ++j) {
            float distance = glm::length(frustumCorners[j] - frustumCenter);
            radius = glm::max(radius, distance);
        } 
        radius = std::ceil(radius * 16.0f) / 16.0f;

        glm::vec3 maxExtents = glm::vec3{radius};
        glm::vec3 minExtents = -maxExtents;
        glm::mat4 cascadeView = glm::lookAt(frustumCenter - lightDir * -minExtents.z, frustumCenter, {0.0f, 1.0f, 0.0f});
        glm::mat4 cascadeProjection = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f, maxExtents.z - minExtents.z);

        cascadeSpace[i] = cascadeProjection * cascadeView;
        splitDepths[i] = (nearPlane + splitDist * clipRange) *  -1.0f;

        lastSplitDist = cascadeSplits[i];
    }
}

void GameRenderer::loadModels(Model const * models, size_t nModels, size_t assetIndex) {
    
    assert(nModels != 0 && "models can not be empty!");

    createVertexBuffer(models, nModels, assetIndex);
    createIndexBuffer(models, nModels, assetIndex);

    Assets & asset = getAssets(assetIndex);

    size_t numTex = 0;
    asset.textureImages.images.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageViews.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageMemories.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);
    for (size_t i = 0; i < nModels; ++i) {
        for (size_t j = 0; j < models[i].meshes.size(); ++j) {
            auto const & material = models[i].materials[models[i].meshes[j].materialIndex];
            auto const & texture = models[i].textures[material.albedoIndex];
            createTextureImage(asset.textureImages.images[numTex], 
                               asset.textureImages.imageMemories[numTex], 
                               texture);
            createTextureImageView(asset.textureImages.imageViews[numTex], 
                                   asset.textureImages.images[numTex], 
                                   texture.mipLevels);
            ++numTex;
        }
    }

    for (size_t i = numTex; i < asset.textureImages.images.size(); ++i) {
        createTextureImage(asset.textureImages.images[i], 
                           asset.textureImages.imageMemories[i], 
                           models[0].textures.back());
        createTextureImageView(asset.textureImages.imageViews[i], 
                               asset.textureImages.images[i], 
                               models[0].textures.back().mipLevels);
    }
}

void GameRenderer::loadCubeMap(const prt::array<Texture, 6>& skybox, size_t assetIndex) {  
    createCubeMapBuffers(assetIndex);
    Assets& ass = getAssets(assetIndex);

    ass.textureImages.images.resize(1);
    ass.textureImages.imageViews.resize(1);
    ass.textureImages.imageMemories.resize(1);
    ass.textureImages.descriptorImageInfos.resize(1);
    createCubeMapImage(ass.textureImages.images[0], 
                       ass.textureImages.imageMemories[0], 
                       skybox);
    createCubeMapImageView(ass.textureImages.imageViews[0], 
                           ass.textureImages.images[0], 
                           skybox[0].mipLevels);
}

// void GameRenderer::createDrawCalls(const prt::vector<Model>& models, 
//                                          const prt::vector<uint32_t>& modelIndices) {
void GameRenderer::createDrawCalls(Model const * models, size_t nModels,
                                   uint32_t const * modelIDs, size_t nModelIDs) {
    // skybox
    {
        GraphicsPipeline & pipeline = graphicsPipelines.scene[skyboxPipelineIndex];
        // Assets & asset = getAssets(pipeline.assetsIndex);
        DrawCall drawCall;
        drawCall.firstIndex = 0;
        drawCall.indexCount = 36;
        pipeline.drawCalls.push_back(drawCall);
    }
    // standard
    {
        GraphicsPipeline & pipeline = graphicsPipelines.scene[standardPipelineIndex];
        prt::vector<uint32_t> imgIdxOffsets = { 0 };
        prt::vector<uint32_t> indexOffsets = { 0 };
        imgIdxOffsets.resize(nModels);
        indexOffsets.resize(nModels);
        for (size_t i = 1; i < nModels; ++i) {
            imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1].meshes.size();
            indexOffsets[i] = indexOffsets[i-1] + models[i-1].indexBuffer.size();
        }
        // Assets & asset = getAssets(graphicsPipelines.scene[standardPipelineIndex].assetsIndex);
        for (size_t i = 0; i < nModelIDs; ++i) {
            const Model& model = models[modelIDs[i]];
            for (auto const & mesh : model.meshes) {
                auto const & material = model.materials[mesh.materialIndex];
                DrawCall drawCall;
                // compute texture indices
                int32_t albedoIndex = material.albedoIndex < 0 ? -1 : imgIdxOffsets[modelIDs[i]] +  material.albedoIndex;
                int32_t normalIndex = material.normalIndex < 0 ? -1 : imgIdxOffsets[modelIDs[i]] + material.normalIndex;
                int32_t specularIndex = material.specularIndex < 0 ? -1 : imgIdxOffsets[modelIDs[i]] + material.specularIndex;
                // push constants
                StandardPushConstants & pc = *reinterpret_cast<StandardPushConstants*>(drawCall.pushConstants.data());
                pc.modelMatrixIdx = i;
                pc.albedoIndex = albedoIndex;
                pc.normalIndex = normalIndex;
                pc.specularIndex = specularIndex;
                pc.baseColor = material.baseColor;
                pc.baseSpecularity = material.baseSpecularity;
                // geometry
                drawCall.firstIndex = indexOffsets[modelIDs[i]] + mesh.startIndex;
                drawCall.indexCount = mesh.numIndices;

                pipeline.drawCalls.push_back(drawCall);
            }
        }
    }
    // shadow map
    {
        GraphicsPipeline & shadowPipeline = graphicsPipelines.offscreen[shadowmapPipelineIndex];
        GraphicsPipeline & standardPipeline = graphicsPipelines.scene[standardPipelineIndex];
        for (auto & standardDrawCall : standardPipeline.drawCalls) {
            shadowPipeline.drawCalls.push_back(standardDrawCall);
        }
    }
}

void GameRenderer::createVertexBuffer(Model const * models, size_t nModels, size_t assetIndex) {
    prt::vector<Vertex> allVertices;
    for (size_t i = 0; i < nModels; ++i) {
        for (size_t j = 0; j < models[i].vertexBuffer.size(); ++j) {
            allVertices.push_back(models[i].vertexBuffer[j]);
        }
    }
    VertexData& data = getAssets(assetIndex).vertexData;
    createAndMapBuffer(allVertices.data(), sizeof(Vertex) * allVertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    
}

void GameRenderer::createIndexBuffer(Model const * models, size_t nModels, size_t assetIndex) {
    prt::vector<uint32_t> allIndices;
    size_t vertexOffset = 0;
    for (size_t i = 0; i < nModels; i++) {
        for (size_t j = 0; j < models[i].indexBuffer.size(); j++) {
            allIndices.push_back(models[i].indexBuffer[j] + vertexOffset);
        }
        vertexOffset += models[i].vertexBuffer.size();
    }
    VertexData& data = getAssets(assetIndex).vertexData;
    createAndMapBuffer(allIndices.data(), sizeof(uint32_t) * allIndices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       data.indexBuffer, 
                       data.indexBufferMemory);
}

void GameRenderer::createCubeMapBuffers(size_t assetIndex) {
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

    VertexData& data = getAssets(assetIndex).vertexData;
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
