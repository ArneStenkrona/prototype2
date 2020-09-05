#include "game_renderer.h"

GameRenderer::GameRenderer(unsigned int width, unsigned int height)
    : VulkanApplication(width, height) {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::createStandardAndShadowGraphicsPipelines(size_t standardAssetIndex, size_t standardUboIndex,
                                                            size_t shadowmapUboIndex, 
                                                            const char * relativeVert, const char * relativeFrag,
                                                            const char * relativeShadowVert,
                                                            VkVertexInputBindingDescription bindingDescription,
                                                            prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                                            int32_t & standardPipeline, int32_t & shadowPipeline) {
    char vert[256] = RESOURCE_PATH;
    char frag[256] = RESOURCE_PATH;
    strcat(vert, relativeVert);
    strcat(frag, relativeFrag);
    standardPipeline = createStandardGraphicsPipeline(standardAssetIndex, standardUboIndex, vert, frag, 
                                                      bindingDescription, attributeDescription);

    vert[0] = '\0';
    strcpy(vert, RESOURCE_PATH);
    strcat(vert, relativeShadowVert);
    shadowPipeline = createShadowmapGraphicsPipeline(standardAssetIndex, shadowmapUboIndex, vert,
                                                     bindingDescription, attributeDescription);
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

int32_t GameRenderer::createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                                    char const * vertexShader, char const * fragmentShader,
                                                    VkVertexInputBindingDescription bindingDescription,
                                                    prt::vector<VkVertexInputAttributeDescription> const & attributeDescription) {
    int32_t pipelineIndex = graphicsPipelines.scene.size();
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
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    VkDescriptorSetLayoutBinding shadowmapLayoutBinding = {};
    shadowmapLayoutBinding.descriptorCount = 1;
    shadowmapLayoutBinding.binding = 3;
    shadowmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
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
    modelPipeline.vertexInputBinding = bindingDescription;
    modelPipeline.vertexInputAttributes.resize(attributeDescription.size());
    size_t inIndx = 0;
    for (auto const & att : attributeDescription) {
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

    return pipelineIndex;
}

int32_t GameRenderer::createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                                   char const * vertexShader,
                                                   VkVertexInputBindingDescription bindingDescription,
                                                   prt::vector<VkVertexInputAttributeDescription> const & attributeDescription) {
    int32_t pipelineIndex = graphicsPipelines.offscreen.size();
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
    pipeline.vertexInputBinding = bindingDescription;
    auto attrib = attributeDescription;
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

    pipeline.extent = offscreenPass.extent;
    pipeline.renderpass = offscreenPass.renderPass;
    pipeline.useColorAttachment = false;
    pipeline.enableDepthBias = true;

    return pipelineIndex;
}

void GameRenderer::bindAssets(Model const * models, size_t nModels,
                              uint32_t const * modelIDs,
                              size_t nModelIDs,
                              Model const * animatedModels,
                              uint32_t const * boneOffsets,
                              size_t nAnimatedModels,
                              uint32_t const * animatedModelIDs,
                              size_t nAnimatedModelIDs,
                              prt::array<Texture, 6> const & skybox) {
    /* skybox */
    size_t skyboxAssetIndex = pushBackAssets();
    size_t skyboxUboIndex = pushBackUniformBufferData(sizeof(SkyboxUBO));
    /* non-animated */
    size_t standardAssetIndex;   
    size_t standardUboIndex;
    size_t shadowMapUboIndex;
    if (nModels != 0) {
        // standard
        standardAssetIndex = pushBackAssets();
        standardUboIndex = pushBackUniformBufferData(sizeof(StandardUBO));
        // shadow map
        shadowMapUboIndex = pushBackUniformBufferData(sizeof(ShadowMapUBO));
    }
    /* animated */
    size_t animatedStandardAssetIndex;
    size_t animatedStandardUboIndex;
    size_t animatedShadowMapUboIndex;
    if (nAnimatedModels != 0) {
        // standard
        animatedStandardAssetIndex = pushBackAssets();
        animatedStandardUboIndex = pushBackUniformBufferData(sizeof(AnimatedStandardUBO));
        // shadow
        animatedShadowMapUboIndex = pushBackUniformBufferData(sizeof(AnimatedShadowMapUBO));
    }
    // Swapchain needs to be updated
    reprepareSwapChain();

    /* skybox */
    loadCubeMap(skybox, skyboxAssetIndex);
    createSkyboxGraphicsPipeline(skyboxAssetIndex, skyboxUboIndex);
    createSkyboxDrawCalls();
    
    /* non-animated */
    if (nModels != 0) {
        loadModels(models, nModels, standardAssetIndex, false);
        createStandardAndShadowGraphicsPipelines(standardAssetIndex, standardUboIndex, shadowMapUboIndex,
                                                 "shaders/standard.vert.spv", "shaders/standard.frag.spv",
                                                 "shaders/shadow_map.vert.spv",
                                                 Model::Vertex::getBindingDescription(),
                                                 Model::Vertex::getAttributeDescriptions(),
                                                 standardPipelineIndex,
                                                 shadowmapPipelineIndex);
        createStandardDrawCalls(models, nModels, modelIDs, nModelIDs, 
                                nullptr, 0,
                                standardPipelineIndex);
        createShadowDrawCalls(shadowmapPipelineIndex, standardPipelineIndex);
    }    

    /* animated */
    if (nAnimatedModels != 0) {
        loadModels(animatedModels, nAnimatedModels, animatedStandardAssetIndex, true);
        createStandardAndShadowGraphicsPipelines(animatedStandardAssetIndex, animatedStandardUboIndex, animatedShadowMapUboIndex,
                                                 "shaders/standard_animated.vert.spv", "shaders/standard.frag.spv",
                                                 "shaders/shadow_map_animated.vert.spv",
                                                 Model::BonedVertex::getBindingDescription(),
                                                 Model::BonedVertex::getAttributeDescriptions(),
                                                 animatedStandardPipelineIndex,
                                                 animatedShadowmapPipelineIndex);

        createStandardDrawCalls(animatedModels, nAnimatedModels, animatedModelIDs, nAnimatedModelIDs, 
                                boneOffsets, sizeof(boneOffsets[0]),
                                animatedStandardPipelineIndex);
        createShadowDrawCalls(animatedShadowmapPipelineIndex, animatedStandardPipelineIndex);
    }

    completeSwapChain();
}

void GameRenderer::update(prt::vector<glm::mat4> const & modelMatrices, 
                          prt::vector<glm::mat4> const & animatedModelMatrices,
                          prt::vector<glm::mat4> const & bones,
                          Camera & camera,
                          DirLight  const & sun,
                          float time) {      
    updateUBOs(modelMatrices, 
               animatedModelMatrices,
               bones,
               camera,
               sun,
               time);
    VulkanApplication::update();   
}

void GameRenderer::updateUBOs(prt::vector<glm::mat4> const & modelMatrices, 
                              prt::vector<glm::mat4> const & animatedModelMatrices,
                              prt::vector<glm::mat4> const & bones,
                              Camera & camera,
                              DirLight  const & sun,
                              float time) {
    glm::mat4 viewMatrix = camera.getViewMatrix();
    int w,h = 0;
    getWindowSize(w, h);
    camera.setProjection(w, h, nearPlane, farPlane);
    glm::mat4 projectionMatrix = camera.getProjectionMatrix();
    // glm::mat4 skyProjectionMatrix = camera.getProjectionMatrix(nearPlane, 1000.0f);
    glm::vec3 viewPosition = camera.getPosition();

    // skybox ubo
    if (skyboxPipelineIndex != -1) {
        updateSkyboxUBO(camera);
    }

    prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> cascadeSpace;
    prt::array<float, NUMBER_SHADOWMAP_CASCADES> splitDepths;
    
    updateCascades(projectionMatrix, viewMatrix, sun.direction, cascadeSpace, splitDepths);
    /* non-animated */
    // standard ubo         
    if (standardPipelineIndex !=  -1) {    
        assert(shadowmapPipelineIndex  != -1);     
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
        // shadow model
        memcpy(shadowUBO.model, standardUBO.model.model, sizeof(standardUBO.model.model[0]) * animatedModelMatrices.size());
        // depth view and projection;
        for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
            shadowUBO.depthVP[i] = cascadeSpace[i];
        }
    }
    /* animated*/
    if (animatedStandardPipelineIndex != -1) {
    assert(animatedShadowmapPipelineIndex != -1);
        auto animatedStandardUboData = getUniformBufferData(graphicsPipelines.scene[animatedStandardPipelineIndex].uboIndex).uboData.data();
        AnimatedStandardUBO & animatedStandardUBO = *reinterpret_cast<AnimatedStandardUBO*>(animatedStandardUboData);
        for (size_t i = 0; i < animatedModelMatrices.size(); ++i) {
            animatedStandardUBO.model.model[i] = animatedModelMatrices[i];
            animatedStandardUBO.model.invTransposeModel[i] = glm::transpose(glm::inverse(animatedModelMatrices[i]));
        }
        // model
        animatedStandardUBO.model.view = viewMatrix;
        animatedStandardUBO.model.proj = projectionMatrix;
        animatedStandardUBO.model.proj[1][1] *= -1;
        animatedStandardUBO.model.viewPosition = viewPosition;
        animatedStandardUBO.model.t = time;
        // lights
        animatedStandardUBO.lighting.sun = sun;
        animatedStandardUBO.lighting.ambientLight = 0.2f;
        animatedStandardUBO.lighting.noPointLights = 0;

        for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
            animatedStandardUBO.lighting.cascadeSpace[i] = cascadeSpace[i];
            animatedStandardUBO.lighting.splitDepths[i/4][i%4] = splitDepths[i];
        }
        // bones
        assert(bones.size() <= NUMBER_MAX_BONES);
        memcpy(&animatedStandardUBO.bones.bones[0], bones.data(), sizeof(glm::mat4) * bones.size()); 
        // shadow map ubo
        auto animatedShadowUboData = getUniformBufferData(graphicsPipelines.offscreen[animatedShadowmapPipelineIndex].uboIndex).uboData.data();
        AnimatedShadowMapUBO & animatedShadowUBO = *reinterpret_cast<AnimatedShadowMapUBO*>(animatedShadowUboData);
        // shadow model
        memcpy(animatedShadowUBO.model, animatedStandardUBO.model.model, sizeof(animatedStandardUBO.model.model[0]) * animatedModelMatrices.size());
        memcpy(animatedShadowUBO.bones.bones, animatedStandardUBO.bones.bones, sizeof(animatedStandardUBO.bones.bones[0]) * bones.size());
        // depth view and projection;
        for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
            animatedShadowUBO.depthVP[i] = cascadeSpace[i];
        }
    }
}

void GameRenderer::updateSkyboxUBO(Camera const & camera) {
    auto skyboxUboData = getUniformBufferData(graphicsPipelines.scene[skyboxPipelineIndex].uboIndex).uboData.data();
    SkyboxUBO & skyboxUBO = *reinterpret_cast<SkyboxUBO*>(skyboxUboData);

    glm::mat4 skyboxViewMatrix = camera.getViewMatrix();
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix * glm::mat4(1.0f);
    skyboxUBO.projection = camera.getProjectionMatrix(nearPlane, 1000.0f);
    skyboxUBO.projection[1][1] *= -1;
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
        // I use negative farPlane as minExtents in Z axis in order to get
        // shadow casters behind camera.
        // This is not ideal but a work-around for now
        glm::mat4 cascadeProjection = glm::ortho(minExtents.x, maxExtents.x, minExtents.y, maxExtents.y, 0.0f - farPlane, maxExtents.z - minExtents.z);

        cascadeSpace[i] = cascadeProjection * cascadeView;
        splitDepths[i] = (nearPlane + splitDist * clipRange) *  -1.0f;

        lastSplitDist = cascadeSplits[i];
    }
}

void GameRenderer::loadModels(Model const * models, size_t nModels, size_t assetIndex,
                              bool animated) {
    assert(nModels != 0 && "models can not be empty!");

    createVertexBuffer(models, nModels, assetIndex, animated);
    createIndexBuffer(models, nModels, assetIndex);

    Assets & asset = getAssets(assetIndex);

    size_t numTex = 0;
    asset.textureImages.images.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageViews.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageMemories.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);
    for (size_t i = 0; i < nModels; ++i) {
        for (size_t j = 0; j < models[i].textures.size(); ++j) {
            auto const & texture = models[i].textures[j];
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

void GameRenderer::loadCubeMap(prt::array<Texture, 6> const & skybox, size_t assetIndex) {  
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

void GameRenderer::createSkyboxDrawCalls() {
    GraphicsPipeline & pipeline = graphicsPipelines.scene[skyboxPipelineIndex];
    DrawCall drawCall;
    drawCall.firstIndex = 0;
    drawCall.indexCount = 36;
    pipeline.drawCalls.push_back(drawCall);
}

void GameRenderer::createStandardDrawCalls(Model    const * models,   size_t nModels,
                                           uint32_t const * modelIDs, size_t nModelIDs,
                                           void const * additionalPushConstants, size_t additionalPushConstantSize,
                                           size_t pipelineIndex) {
    GraphicsPipeline & pipeline = graphicsPipelines.scene[pipelineIndex];
    prt::vector<uint32_t> imgIdxOffsets = { 0 };
    prt::vector<uint32_t> indexOffsets = { 0 };
    imgIdxOffsets.resize(nModels);
    indexOffsets.resize(nModels);
    for (size_t i = 1; i < nModels; ++i) {
        imgIdxOffsets[i] = imgIdxOffsets[i-1] + models[i-1].textures.size();
        indexOffsets[i] = indexOffsets[i-1] + models[i-1].indexBuffer.size();
    }

    for (size_t i = 0; i < nModelIDs; ++i) {
        const Model& model = models[modelIDs[i]];

        for (auto const & mesh : model.meshes) {
            auto const & material = model.materials[mesh.materialIndex];
            DrawCall drawCall;
            // compute texture indices
            int32_t albedoIndex = material.albedoIndex     < 0 ? -1 : imgIdxOffsets[modelIDs[i]] +  material.albedoIndex;
            int32_t normalIndex = material.normalIndex     < 0 ? -1 : imgIdxOffsets[modelIDs[i]] + material.normalIndex;
            int32_t specularIndex = material.specularIndex < 0 ? -1 : imgIdxOffsets[modelIDs[i]] + material.specularIndex;
            // push constants
            StandardPushConstants & pc = *reinterpret_cast<StandardPushConstants*>(drawCall.pushConstants.data());
            pc.modelMatrixIdx = i;
            pc.albedoIndex = albedoIndex;
            pc.normalIndex = normalIndex;
            pc.specularIndex = specularIndex;
            pc.baseColor = material.baseColor;
            pc.baseSpecularity = material.baseSpecularity;
            // add any additional push constants
            if (additionalPushConstantSize != 0) {
                memcpy(pc.additionalData, 
                       &static_cast<unsigned char const *>(additionalPushConstants)[i * additionalPushConstantSize], 
                       additionalPushConstantSize);
            }
            // geometry
            drawCall.firstIndex = indexOffsets[modelIDs[i]] + mesh.startIndex;
            drawCall.indexCount = mesh.numIndices;

            pipeline.drawCalls.push_back(drawCall);
        }
    }
}
void GameRenderer::createShadowDrawCalls(size_t shadowPipelineIndex, size_t pipelineIndex) {
    GraphicsPipeline & shadowPipeline = graphicsPipelines.offscreen[shadowPipelineIndex];
    GraphicsPipeline & standardPipeline = graphicsPipelines.scene[pipelineIndex];
    for (auto & standardDrawCall : standardPipeline.drawCalls) {
        shadowPipeline.drawCalls.push_back(standardDrawCall);
    }
}

void GameRenderer::createVertexBuffer(Model const * models, size_t nModels, size_t assetIndex,
                                      bool animated) {
    size_t vertexSize = animated ? sizeof(Model::BonedVertex) : sizeof(Model::Vertex);
    prt::vector<unsigned char> vertexData;
    for (size_t i = 0; i < nModels; ++i) {
        size_t prevSize = vertexData.size();
        vertexData.resize(prevSize + vertexSize * models[i].vertexBuffer.size());
        unsigned char* dest = vertexData.data() + prevSize;
        if (animated) {
            assert(models[i].vertexBuffer.size() == models[i].vertexBoneBuffer.size());
            for (size_t j = 0; j < models[i].vertexBuffer.size(); ++j) {
                memcpy(dest, &models[i].vertexBuffer[j], sizeof(Model::Vertex));
                memcpy(dest + sizeof(Model::Vertex), &models[i].vertexBoneBuffer[j], sizeof(Model::BoneData));
                dest += vertexSize;
            }
        } else {
            memcpy(dest, models[i].vertexBuffer.data(), vertexSize * models[i].vertexBuffer.size());
        }
    }
    VertexData& data = getAssets(assetIndex).vertexData;
    createAndMapBuffer(vertexData.data(), vertexSize * vertexData.size(),
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
