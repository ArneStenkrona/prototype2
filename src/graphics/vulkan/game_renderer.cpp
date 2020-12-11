#include "game_renderer.h"

#include "src/util/math_util.h"

GameRenderer::GameRenderer(unsigned int width, unsigned int height)
    : VulkanApplication(width, height) {
}

GameRenderer::~GameRenderer() {
}

void GameRenderer::createStandardAndShadowPipelines(size_t standardAssetIndex, size_t standardUboIndex,
                                                            size_t shadowmapUboIndex, 
                                                            const char * relativeVert, const char * relativeFrag,
                                                            const char * relativeTransparentFrag,
                                                            const char * relativeShadowVert,
                                                            VkVertexInputBindingDescription bindingDescription,
                                                            prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                                            int32_t & standardPipeline, 
                                                            int32_t & transparentPipeline,
                                                            int32_t & shadowPipeline) {
    char vert[256] = RESOURCE_PATH;
    char frag[256] = RESOURCE_PATH;
    char transparentFrag[256] = RESOURCE_PATH;
    strcat(vert, relativeVert);
    strcat(frag, relativeFrag);
    strcat(transparentFrag, relativeTransparentFrag);
    standardPipeline = createStandardPipeline(standardAssetIndex, standardUboIndex, vert, frag,
                                                      bindingDescription, attributeDescription, false);
    transparentPipeline = createStandardPipeline(standardAssetIndex, standardUboIndex, vert, transparentFrag,
                                                         bindingDescription, attributeDescription, true);

    vert[0] = '\0';
    strcpy(vert, RESOURCE_PATH);
    strcat(vert, relativeShadowVert);
    shadowPipeline = createShadowmapPipeline(standardAssetIndex, shadowmapUboIndex, vert,
                                                     bindingDescription, attributeDescription);
}

void GameRenderer::createCompositionPipeline() {
    compositionPipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.type = PIPELINE_TYPE_COMPOSITION;
    pipeline.subpass = 2;

    pipeline.assetsIndex = pushBackAssets();
    // Assets const & asset = getAssets(pipeline.assetsIndex);

    // accumulation and revealage for transparency
    pipeline.descriptorSetLayoutBindings.resize(2);

    pipeline.descriptorSetLayoutBindings[0].descriptorCount = 1;
    pipeline.descriptorSetLayoutBindings[0].binding = 0;
    pipeline.descriptorSetLayoutBindings[0].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline.descriptorSetLayoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    pipeline.descriptorSetLayoutBindings[0].pImmutableSamplers = nullptr;

    pipeline.descriptorSetLayoutBindings[1].descriptorCount = 1;
    pipeline.descriptorSetLayoutBindings[1].binding = 1;
    pipeline.descriptorSetLayoutBindings[1].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    pipeline.descriptorSetLayoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    pipeline.descriptorSetLayoutBindings[1].pImmutableSamplers = nullptr;

    // Descriptor pools
    // accumulation and revealage for transparency
    pipeline.descriptorPoolSizes.resize(2);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    pipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
    pipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        pipeline.descriptorWrites[i].resize(2, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0] = {};
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].dstArrayElement = 0;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
        pipeline.descriptorWrites[i][0].pBufferInfo = 0;
        // pipeline.descriptorWrites[i][0].pImageInfo = &accumulationDescriptors[i];

        pipeline.descriptorWrites[i][1] = {};
        pipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][1].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][1].dstBinding = 1;
        pipeline.descriptorWrites[i][1].dstArrayElement = 0;
        pipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT;
        pipeline.descriptorWrites[i][1].descriptorCount = 1;
        pipeline.descriptorWrites[i][1].pBufferInfo = 0;
        // pipeline.descriptorWrites[i][1].pImageInfo = &revealageDescriptors[i];
    }

    // Vertex input
    pipeline.vertexInputBinding.binding = 0;
    pipeline.vertexInputBinding.stride = sizeof(glm::vec3);
    pipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipeline.vertexInputAttributes.resize(1);
    pipeline.vertexInputAttributes[0].binding = 0;
    pipeline.vertexInputAttributes[0].location = 0;
    pipeline.vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    pipeline.vertexInputAttributes[0].offset = 0;

    auto & shaderStages = pipeline.shaderStages;
    shaderStages.resize(2);
    
    char vert[256] = RESOURCE_PATH;
    char frag[256] = RESOURCE_PATH;
    strcat(vert, "shaders/composition.vert.spv");
    strcat(frag, "shaders/composition.frag.spv");

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, vert);

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName[0] = '\0';
    strcat(shaderStages[1].pName, "main");
    shaderStages[1].shader[0] = '\0';
    strcat(shaderStages[1].shader, frag);

    pipeline.extent = swapChainExtent;
    pipeline.renderpass = scenePass;
    pipeline.useColorAttachment = true;
    pipeline.enableDepthBias = false;

    prt::vector<glm::vec3> vertices;

    vertices.resize(4);
    vertices[0] = glm::vec3{-1.0f, -1.0f, 0.0f};
    vertices[1] = glm::vec3{1.0f, -1.0f, 0.0f};
    vertices[2] = glm::vec3{1.0f, 1.0f, 0.0f};
    vertices[3] = glm::vec3{-1.0f, 1.0f, 0.0f};

    VertexData& data = getAssets(pipeline.assetsIndex).vertexData;
    createAndMapBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    

    prt::vector<uint32_t> indices = { 0, 2, 1,
                                      0, 3, 2 };
    
    createAndMapBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       data.indexBuffer, 
                       data.indexBufferMemory);
}

void GameRenderer::createGridPipeline(size_t assetIndex, size_t uboIndex) {
    gridPipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.subpass = 1;
    pipeline.type = PIPELINE_TYPE_TRANSPARENT;
    pipeline.renderGroup = EDITOR_RENDER_GROUP;

    pipeline.assetsIndex = assetIndex;
    pipeline.uboIndex = uboIndex;
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;


    pipeline.descriptorSetLayoutBindings.resize(1);
    pipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(1);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
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
        // pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];
    }

    // Vertex input
    pipeline.vertexInputBinding.binding = 0;
    pipeline.vertexInputBinding.stride = sizeof(glm::vec3);
    pipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipeline.vertexInputAttributes.resize(1);
    pipeline.vertexInputAttributes[0].binding = 0;
    pipeline.vertexInputAttributes[0].location = 0;
    pipeline.vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    pipeline.vertexInputAttributes[0].offset = 0;

    auto & shaderStages = pipeline.shaderStages;
    shaderStages.resize(2);

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, RESOURCE_PATH);
    strcat(shaderStages[0].shader, "shaders/grid.vert.spv");

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName[0] = '\0';
    strcat(shaderStages[1].pName, "main");
    shaderStages[1].shader[0] = '\0';
    strcat(shaderStages[1].shader, RESOURCE_PATH);
    strcat(shaderStages[1].shader, "shaders/grid.frag.spv");

    pipeline.extent = swapChainExtent;
    pipeline.renderpass = scenePass;
    pipeline.useColorAttachment = true;
    pipeline.enableDepthBias = false;
}

void GameRenderer::createSkyboxPipeline(size_t assetIndex, size_t uboIndex) {
    skyboxPipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.subpass = 0;
    pipeline.type = PIPELINE_TYPE_OPAQUE;

    pipeline.assetsIndex = assetIndex;
    pipeline.uboIndex = uboIndex;
    Assets const & asset = getAssets(assetIndex);
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding cubeMapSamplerLayoutBinding = {};
    cubeMapSamplerLayoutBinding.descriptorCount = 1;
    cubeMapSamplerLayoutBinding.binding = 1;
    cubeMapSamplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    cubeMapSamplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    cubeMapSamplerLayoutBinding.pImmutableSamplers = nullptr;

    pipeline.descriptorSetLayoutBindings.resize(2);
    pipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    pipeline.descriptorSetLayoutBindings[1] = cubeMapSamplerLayoutBinding;
    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(2);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    pipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
        pipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        pipeline.descriptorBufferInfos[i].offset = 0;
        pipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();

        pipeline.descriptorWrites[i].resize(2, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].dstArrayElement = 0;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
        // pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];

        pipeline.descriptorWrites[i][1] = {};
        pipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][1].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][1].dstBinding = 1;
        pipeline.descriptorWrites[i][1].dstArrayElement = 0;
        pipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pipeline.descriptorWrites[i][1].descriptorCount = 1;
        pipeline.descriptorWrites[i][1].pBufferInfo = 0;
        pipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();
    }

    // Vertex input
    pipeline.vertexInputBinding.binding = 0;
    pipeline.vertexInputBinding.stride = sizeof(glm::vec3);
    pipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipeline.vertexInputAttributes.resize(1);
    pipeline.vertexInputAttributes[0].binding = 0;
    pipeline.vertexInputAttributes[0].location = 0;
    pipeline.vertexInputAttributes[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    pipeline.vertexInputAttributes[0].offset = 0;

    auto & shaderStages = pipeline.shaderStages;
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

    pipeline.extent = swapChainExtent;
    pipeline.renderpass = scenePass;
    pipeline.useColorAttachment = false;
    pipeline.enableDepthBias = true;
}

void GameRenderer::createBillboardPipeline(size_t assetIndex, size_t uboIndex) {
    billboardPipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.subpass = 1;
    pipeline.type = PIPELINE_TYPE_TRANSPARENT;

    pipeline.assetsIndex = assetIndex;
    pipeline.uboIndex = uboIndex;
    Assets const & asset = getAssets(assetIndex);
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

    // Descriptor set layout
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // textures
    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.descriptorCount = NUMBER_SUPPORTED_BILLBOARD_TEXTURES;
    textureLayoutBinding.binding = 1;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureLayoutBinding.pImmutableSamplers = nullptr;
    // texture sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;

    pipeline.descriptorSetLayoutBindings.resize(3);
    pipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    pipeline.descriptorSetLayoutBindings[1] = textureLayoutBinding;
    pipeline.descriptorSetLayoutBindings[2] = samplerLayoutBinding;
    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(3);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    pipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_BILLBOARD_TEXTURES * swapChainImages.size());
    pipeline.descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pipeline.descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    
    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); i++) { 
        pipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        pipeline.descriptorBufferInfos[i].offset = 0;
        pipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();

        pipeline.descriptorWrites[i].resize(3, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].dstArrayElement = 0;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
        // pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];

        pipeline.descriptorWrites[i][1] = {};
        pipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][1].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][1].dstBinding = 1;
        pipeline.descriptorWrites[i][1].dstArrayElement = 0;
        pipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pipeline.descriptorWrites[i][1].descriptorCount = NUMBER_SUPPORTED_BILLBOARD_TEXTURES;
        pipeline.descriptorWrites[i][1].pBufferInfo = 0;
        pipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();

        // VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = textureSampler;

        pipeline.descriptorWrites[i][2] = {};
        pipeline.descriptorWrites[i][2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][2].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][2].dstBinding = 2;
        pipeline.descriptorWrites[i][2].dstArrayElement = 0;
        pipeline.descriptorWrites[i][2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        pipeline.descriptorWrites[i][2].descriptorCount = 1;
        pipeline.descriptorWrites[i][2].pBufferInfo = 0;
        pipeline.descriptorWrites[i][2].pImageInfo = &samplerInfo;
    }

    // Vertex input
    pipeline.vertexInputBinding.binding = 0;
    pipeline.vertexInputBinding.stride = 2 * sizeof(glm::vec2);
    pipeline.vertexInputBinding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

    pipeline.vertexInputAttributes.resize(2);
    pipeline.vertexInputAttributes[0].binding = 0;
    pipeline.vertexInputAttributes[0].location = 0;
    pipeline.vertexInputAttributes[0].format = VK_FORMAT_R32G32_SFLOAT;
    pipeline.vertexInputAttributes[0].offset = 0;

    pipeline.vertexInputAttributes[1].binding = 0;
    pipeline.vertexInputAttributes[1].location = 1;
    pipeline.vertexInputAttributes[1].format = VK_FORMAT_R32G32_SFLOAT;
    pipeline.vertexInputAttributes[1].offset = 8;

    auto & shaderStages = pipeline.shaderStages;
    shaderStages.resize(2);

    shaderStages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
    shaderStages[0].pName[0] = '\0';
    strcat(shaderStages[0].pName, "main");
    shaderStages[0].shader[0] = '\0';
    strcat(shaderStages[0].shader, RESOURCE_PATH);
    strcat(shaderStages[0].shader, "shaders/billboard.vert.spv");

    shaderStages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
    shaderStages[1].pName[0] = '\0';
    strcat(shaderStages[1].pName, "main");
    shaderStages[1].shader[0] = '\0';
    strcat(shaderStages[1].shader, RESOURCE_PATH);
    strcat(shaderStages[1].shader, "shaders/billboard.frag.spv");

    pipeline.extent = swapChainExtent;
    pipeline.renderpass = scenePass;
    pipeline.useColorAttachment = true;
    pipeline.enableDepthBias = false;
}

int32_t GameRenderer::createStandardPipeline(size_t assetIndex, size_t uboIndex,
                                                    char const * vertexShader, char const * fragmentShader,
                                                    VkVertexInputBindingDescription bindingDescription,
                                                    prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                                    bool transparent) {
    int32_t pipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.subpass = transparent ? 1 : 0;
    pipeline.type = transparent ? PIPELINE_TYPE_TRANSPARENT : PIPELINE_TYPE_OPAQUE;

    pipeline.assetsIndex = assetIndex;
    pipeline.uboIndex = uboIndex;
    Assets const & asset = getAssets(assetIndex);
    UniformBufferData const & uniformBufferData = getUniformBufferData(uboIndex);

    /* Descriptor set layout */
    // ubo
    VkDescriptorSetLayoutBinding uboLayoutBinding = {};
    uboLayoutBinding.binding = 0;
    uboLayoutBinding.descriptorCount = 1;
    uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    uboLayoutBinding.pImmutableSamplers = nullptr;
    uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;
    // textures
    VkDescriptorSetLayoutBinding textureLayoutBinding = {};
    textureLayoutBinding.descriptorCount = NUMBER_SUPPORTED_TEXTURES;
    textureLayoutBinding.binding = 1;
    textureLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    textureLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    textureLayoutBinding.pImmutableSamplers = nullptr;
    // texture sampler
    VkDescriptorSetLayoutBinding samplerLayoutBinding = {};
    samplerLayoutBinding.descriptorCount = 1;
    samplerLayoutBinding.binding = 2;
    samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
    samplerLayoutBinding.pImmutableSamplers = nullptr;
    // shadowmap
    VkDescriptorSetLayoutBinding shadowmapLayoutBinding = {};
    shadowmapLayoutBinding.descriptorCount = 1;
    shadowmapLayoutBinding.binding = 3;
    shadowmapLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    shadowmapLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    shadowmapLayoutBinding.pImmutableSamplers = nullptr;
    
    pipeline.descriptorSetLayoutBindings.resize(4);
    pipeline.descriptorSetLayoutBindings[0] = uboLayoutBinding;
    pipeline.descriptorSetLayoutBindings[1] = textureLayoutBinding;
    pipeline.descriptorSetLayoutBindings[2] = samplerLayoutBinding;
    pipeline.descriptorSetLayoutBindings[3] = shadowmapLayoutBinding;

    // Descriptor pools
    pipeline.descriptorPoolSizes.resize(4);
    pipeline.descriptorPoolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    pipeline.descriptorPoolSizes[0].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    pipeline.descriptorPoolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    pipeline.descriptorPoolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_TEXTURES * swapChainImages.size());
    pipeline.descriptorPoolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    pipeline.descriptorPoolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());
    pipeline.descriptorPoolSizes[3].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    pipeline.descriptorPoolSizes[3].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    // Descriptor sets
    pipeline.descriptorSets.resize(swapChainImages.size());
    pipeline.descriptorWrites.resize(swapChainImages.size());
    for (size_t i = 0; i < swapChainImages.size(); ++i) {
        pipeline.descriptorBufferInfos[i].buffer = uniformBufferData.uniformBuffers[i];
        pipeline.descriptorBufferInfos[i].offset = 0;
        pipeline.descriptorBufferInfos[i].range = uniformBufferData.uboData.size();
        
        pipeline.descriptorWrites[i].resize(4, VkWriteDescriptorSet{});
        
        pipeline.descriptorWrites[i][0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][0].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][0].dstBinding = 0;
        pipeline.descriptorWrites[i][0].dstArrayElement = 0;
        pipeline.descriptorWrites[i][0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        pipeline.descriptorWrites[i][0].descriptorCount = 1;
        // pipeline.descriptorWrites[i][0].pBufferInfo = &pipeline.descriptorBufferInfos[i];

        pipeline.descriptorWrites[i][1] = {};
        pipeline.descriptorWrites[i][1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][1].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][1].dstBinding = 1;
        pipeline.descriptorWrites[i][1].dstArrayElement = 0;
        pipeline.descriptorWrites[i][1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        pipeline.descriptorWrites[i][1].descriptorCount = NUMBER_SUPPORTED_TEXTURES;
        pipeline.descriptorWrites[i][1].pBufferInfo = 0;
        pipeline.descriptorWrites[i][1].pImageInfo = asset.textureImages.descriptorImageInfos.data();

        // VkDescriptorImageInfo samplerInfo = {};
        samplerInfo.sampler = textureSampler;

        pipeline.descriptorWrites[i][2] = {};
        pipeline.descriptorWrites[i][2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][2].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][2].dstBinding = 2;
        pipeline.descriptorWrites[i][2].dstArrayElement = 0;
        pipeline.descriptorWrites[i][2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        pipeline.descriptorWrites[i][2].descriptorCount = 1;
        pipeline.descriptorWrites[i][2].pBufferInfo = 0;
        pipeline.descriptorWrites[i][2].pImageInfo = &samplerInfo;

        pipeline.descriptorWrites[i][3] = {};
        pipeline.descriptorWrites[i][3].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        // pipeline.descriptorWrites[i][3].dstSet = pipeline.descriptorSets[i];
        pipeline.descriptorWrites[i][3].dstBinding = 3;
        pipeline.descriptorWrites[i][3].dstArrayElement = 0;
        pipeline.descriptorWrites[i][3].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        pipeline.descriptorWrites[i][3].descriptorCount = 1;
        pipeline.descriptorWrites[i][3].pBufferInfo = 0;
        pipeline.descriptorWrites[i][3].pImageInfo = &offscreenPass.descriptors[i];
    }

    // Vertex input
    pipeline.vertexInputBinding = bindingDescription;
    pipeline.vertexInputAttributes.resize(attributeDescription.size());
    size_t inIndx = 0;
    for (auto const & att : attributeDescription) {
        pipeline.vertexInputAttributes[inIndx] = att;
        ++inIndx;
    }
    auto & shaderStages = pipeline.shaderStages;
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

    pipeline.extent = swapChainExtent;
    pipeline.renderpass = scenePass;
    pipeline.useColorAttachment = true;
    pipeline.enableDepthBias = false;

    return pipelineIndex;
}

int32_t GameRenderer::createShadowmapPipeline(size_t assetIndex, size_t uboIndex,
                                                   char const * vertexShader,
                                                   VkVertexInputBindingDescription bindingDescription,
                                                   prt::vector<VkVertexInputAttributeDescription> const & attributeDescription) {
    int32_t pipelineIndex = graphicsPipelines.size();
    graphicsPipelines.push_back(GraphicsPipeline{});
    GraphicsPipeline& pipeline = graphicsPipelines.back();

    pipeline.type = PIPELINE_TYPE_OFFSCREEN;
    pipeline.subpass = 0;

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

// TODO: Split this function to decouple pipeline
// creation from asset binding
void GameRenderer::bindAssets(Model const * models, size_t nModels,
                              uint32_t const * modelIDs,
                              size_t nModelIDs,
                              Model const * animatedModels,
                              uint32_t const * boneOffsets,
                              size_t nAnimatedModels,
                              uint32_t const * animatedModelIDs,
                              size_t nAnimatedModelIDs,
                              Billboard const * billboards,
                              size_t nBillboards,
                              Texture const * textures,
                              size_t nTextures,
                              prt::array<Texture, 6> const & skybox) {
    /* grid */
    size_t gridAssetIndex = pushBackAssets();
    size_t gridUboIndex = pushBackUniformBufferData(sizeof(GridUBO));
    /* skybox */
    size_t skyboxAssetIndex = pushBackAssets();
    size_t skyboxUboIndex = pushBackUniformBufferData(sizeof(SkyboxUBO));
    /* non-animated */
    size_t standardAssetIndex;   
    size_t standardUboIndex;
    size_t shadowMapUboIndex;
    // standard
    standardAssetIndex = pushBackAssets();
    standardUboIndex = pushBackUniformBufferData(sizeof(StandardUBO));
    // shadow map
    shadowMapUboIndex = pushBackUniformBufferData(sizeof(ShadowMapUBO));

    /* animated */
    size_t animatedStandardAssetIndex;
    size_t animatedStandardUboIndex;
    size_t animatedShadowMapUboIndex;
    // standard
    animatedStandardAssetIndex = pushBackAssets();
    animatedStandardUboIndex = pushBackUniformBufferData(sizeof(AnimatedStandardUBO));
    // shadow
    animatedShadowMapUboIndex = pushBackUniformBufferData(sizeof(AnimatedShadowMapUBO));

    /* billboards */
    size_t billboardAssetIndex;
    size_t billboardUboIndex;
    // if (nBillboards) {
        billboardAssetIndex = pushBackAssets();
        billboardUboIndex = pushBackUniformBufferData(sizeof(BillboardUBO));
    // }

    // Swapchain needs to be updated
    reprepareSwapChain();

    /* grid */
    createGridPipeline(gridAssetIndex, gridUboIndex);
    createGridBuffers(gridAssetIndex);
    createGridDrawCalls();

    /* skybox */
    loadCubeMap(skybox, skyboxAssetIndex);
    createSkyboxPipeline(skyboxAssetIndex, skyboxUboIndex);
    createSkyboxDrawCalls();
    
    /* non-animated */
    prt::hash_map<int32_t, int32_t> standardTextureIndices;
    loadModels(models, nModels, textures, nTextures, standardAssetIndex, false, standardTextureIndices);
    createStandardAndShadowPipelines(standardAssetIndex, standardUboIndex, shadowMapUboIndex,
                                             "shaders/standard.vert.spv", "shaders/standard.frag.spv",
                                             "shaders/standard_transparent.frag.spv",
                                             "shaders/shadow_map.vert.spv",
                                             Model::Vertex::getBindingDescription(),
                                             Model::Vertex::getAttributeDescriptions(),
                                             standardPipelineIndex,
                                             transparentPipelineIndex,
                                             shadowmapPipelineIndex);

    /* animated */
    prt::hash_map<int32_t, int32_t> animatedTextureIndices;
    loadModels(animatedModels, nAnimatedModels, textures, nTextures, animatedStandardAssetIndex, true, animatedTextureIndices);
    createStandardAndShadowPipelines(animatedStandardAssetIndex, animatedStandardUboIndex, animatedShadowMapUboIndex,
                                                "shaders/standard_animated.vert.spv", "shaders/standard.frag.spv",
                                                "shaders/standard_transparent.frag.spv",
                                                "shaders/shadow_map_animated.vert.spv",
                                                Model::BonedVertex::getBindingDescription(),
                                                Model::BonedVertex::getAttributeDescriptions(),
                                                animatedStandardPipelineIndex,
                                                animatedTransparentPipelineIndex,
                                                animatedShadowmapPipelineIndex);

    /* water */
    char vert[256] = RESOURCE_PATH;
    char frag[256] = RESOURCE_PATH;
    strcat(vert, "shaders/water.vert.spv");
    strcat(frag, "shaders/standard_transparent.frag.spv");
    waterPipelineIndex = createStandardPipeline(standardAssetIndex, standardUboIndex, vert, frag,
                                                        Model::Vertex::getBindingDescription(),
                                                        Model::Vertex::getAttributeDescriptions(), true);

    prt::vector<DrawCall> temp;
    createModelDrawCalls(models, nModels, modelIDs, nModelIDs,
                         animatedModels, nAnimatedModels, animatedModelIDs, nAnimatedModelIDs,
                         boneOffsets, 
                         standardTextureIndices,
                         animatedTextureIndices,
                         graphicsPipelines[standardPipelineIndex].drawCalls,
                         graphicsPipelines[transparentPipelineIndex].drawCalls,
                         graphicsPipelines[animatedStandardPipelineIndex].drawCalls,
                         graphicsPipelines[animatedTransparentPipelineIndex].drawCalls,
                         graphicsPipelines[waterPipelineIndex].drawCalls,
                         graphicsPipelines[shadowmapPipelineIndex].drawCalls,
                         graphicsPipelines[animatedShadowmapPipelineIndex].drawCalls);

    /* billboard */
    // if (nBillboards != 0) {
        prt::hash_map<int32_t, int32_t> textureIndices;
        loadBillboards(billboards, nBillboards, textures, nTextures, billboardAssetIndex, textureIndices);
        createBillboardPipeline(billboardAssetIndex, billboardUboIndex);

        createBillboardBuffers(billboardAssetIndex);
        createBillboardDrawCalls(billboards, nBillboards, textureIndices);
    // }

    /* composition */
    createCompositionPipeline();
    createCompositionDrawCalls(compositionPipelineIndex);

    completeSwapChain();
}

void GameRenderer::update(prt::vector<glm::mat4> const & modelMatrices, 
                          prt::vector<glm::mat4> const & animatedModelMatrices,
                          prt::vector<glm::mat4> const & bones,
                          prt::vector<glm::vec4> const & billboardPositions,
                          prt::vector<glm::vec4> const & billboardColors,
                          Camera & camera,
                          SkyLight  const & sun,
                          prt::vector<PointLight> const & pointLights,
                          prt::vector<PackedBoxLight> const & boxLights,
                          float t) {      
    updateUBOs(modelMatrices, 
               animatedModelMatrices,
               bones,
               billboardPositions,
               billboardColors,
               camera,
               sun,
               pointLights,
               boxLights,
               t);
}

void GameRenderer::updateUBOs(prt::vector<glm::mat4> const & modelMatrices, 
                              prt::vector<glm::mat4> const & animatedModelMatrices,
                              prt::vector<glm::mat4> const & bones,
                              prt::vector<glm::vec4> const & billboardPositions,
                              prt::vector<glm::vec4> const & billboardColors,
                              Camera & camera,
                              SkyLight  const & sun,
                              prt::vector<PointLight> const & pointLights,
                              prt::vector<PackedBoxLight> const & boxLights,
                              float t) {
    glm::mat4 viewMatrix = camera.getViewMatrix();
    int w,h = 0;
    getWindowSize(w, h);
    camera.setProjection(w, h, nearPlane, farPlane);
    glm::mat4 projectionMatrix = camera.getProjectionMatrix();
    glm::vec3 viewPosition = camera.getPosition();

    // grid ubo
    if (gridPipelineIndex != -1) {
        auto gridUboData = getUniformBufferData(graphicsPipelines[gridPipelineIndex].uboIndex).uboData.data();
        GridUBO & gridUBO = *reinterpret_cast<GridUBO*>(gridUboData);
        gridUBO.view = viewMatrix;
        gridUBO.proj = projectionMatrix;
        gridUBO.viewPosition = glm::vec4(viewPosition, 1.0f);
        gridUBO.proj[1][1] *= -1;
        gridUBO.nearPlane = nearPlane;
        gridUBO.farPlane = farPlane;
    }

    // skybox ubo
    if (skyboxPipelineIndex != -1) {
        updateSkyboxUBO(camera, sun);
    }

    if (billboardPipelineIndex != -1) {
        updateBillboardUBO(camera, billboardPositions, billboardColors);
    }

    prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> cascadeSpace;
    prt::array<float, NUMBER_SHADOWMAP_CASCADES> splitDepths;
    
    updateCascades(projectionMatrix, viewMatrix, sun.direction, cascadeSpace, splitDepths);
    /* non-animated */
    // standard ubo         
    if (standardPipelineIndex !=  -1) {    
        assert(shadowmapPipelineIndex  != -1);     
        auto standardUboData = getUniformBufferData(graphicsPipelines[standardPipelineIndex].uboIndex).uboData.data();
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

        standardUBO.model.t = t;

        // lights
        standardUBO.lighting.sun.color = sun.color;
        standardUBO.lighting.sun.direction = sun.direction;
        standardUBO.lighting.ambientLight = 0.2f;
        standardUBO.lighting.noPointLights = glm::min(size_t(NUMBER_SUPPORTED_POINTLIGHTS), pointLights.size());
        for (unsigned int i = 0; i < standardUBO.lighting.noPointLights; ++i) {
            standardUBO.lighting.pointLights[i] = pointLights[i];
        }
        standardUBO.lighting.noBoxLights = glm::min(size_t(NUMBER_SUPPORTED_BOXLIGHTS), boxLights.size());
        for (unsigned int i = 0; i < standardUBO.lighting.noBoxLights; ++i) {
            standardUBO.lighting.boxLights[i] = boxLights[i];
        }

        for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
            standardUBO.lighting.cascadeSpace[i] = cascadeSpace[i];
            standardUBO.lighting.splitDepths[i/4][i%4] = splitDepths[i];
        }
        // shadow map ubo
        auto shadowUboData = getUniformBufferData(graphicsPipelines[shadowmapPipelineIndex].uboIndex).uboData.data();
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
        auto animatedStandardUboData = getUniformBufferData(graphicsPipelines[animatedStandardPipelineIndex].uboIndex).uboData.data();
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

        animatedStandardUBO.model.t = t;
        // lights
        animatedStandardUBO.lighting.sun.color = sun.color;
        animatedStandardUBO.lighting.sun.direction = sun.direction;
        animatedStandardUBO.lighting.ambientLight = 0.2f;
        animatedStandardUBO.lighting.noPointLights = glm::min(size_t(NUMBER_SUPPORTED_POINTLIGHTS), pointLights.size());
        for (unsigned int i = 0; i < animatedStandardUBO.lighting.noPointLights; ++i) {
            animatedStandardUBO.lighting.pointLights[i] = pointLights[i];
        }
        animatedStandardUBO.lighting.noBoxLights = glm::min(size_t(NUMBER_SUPPORTED_BOXLIGHTS), boxLights.size());
        for (unsigned int i = 0; i < animatedStandardUBO.lighting.noBoxLights; ++i) {
            animatedStandardUBO.lighting.boxLights[i] = boxLights[i];
        }

        for (unsigned int i = 0; i < cascadeSpace.size(); ++i) {
            animatedStandardUBO.lighting.cascadeSpace[i] = cascadeSpace[i];
            animatedStandardUBO.lighting.splitDepths[i/4][i%4] = splitDepths[i];
        }
        // bones
        assert(bones.size() <= NUMBER_MAX_BONES);
        memcpy(&animatedStandardUBO.bones.bones[0], bones.data(), sizeof(glm::mat4) * bones.size()); 
        // shadow map ubo
        auto animatedShadowUboData = getUniformBufferData(graphicsPipelines[animatedShadowmapPipelineIndex].uboIndex).uboData.data();
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

void GameRenderer::updateSkyboxUBO(Camera const & camera, SkyLight const & sky) {
    auto skyboxUboData = getUniformBufferData(graphicsPipelines[skyboxPipelineIndex].uboIndex).uboData.data();
    SkyboxUBO & skyboxUBO = *reinterpret_cast<SkyboxUBO*>(skyboxUboData);

    glm::mat4 skyboxViewMatrix = camera.getViewMatrix();
    skyboxViewMatrix[3][0] = 0.0f;
    skyboxViewMatrix[3][1] = 0.0f;
    skyboxViewMatrix[3][2] = 0.0f;

    skyboxUBO.model = skyboxViewMatrix;// * glm::mat4(1.0f);
    skyboxUBO.projection = camera.getProjectionMatrix(nearPlane, 1000.0f);
    skyboxUBO.projection[1][1] *= -1;

    skyboxUBO.skyRotation = glm::rotate(glm::mat4(1.0f), -sky.phase, glm::vec3(1.0f,0.0f,0.0f));
    skyboxUBO.sunDirection = glm::vec4(glm::normalize(sky.direction), 0.0f);
    skyboxUBO.dayColor = glm::vec4(sky.dayColor, 1.0f);
    skyboxUBO.nightColor = glm::vec4(sky.nightColor, 1.0f);
    skyboxUBO.sunColor = glm::vec4(sky.sunColor, 1.0f);
    skyboxUBO.sunEdgeColor = glm::vec4(sky.sunEdgeColor, 1.0f);
    skyboxUBO.sunsetriseColor = glm::vec4(sky.sunsetriseColor, 1.0f);
    skyboxUBO.distToNoon = sky.distToNoon;
}

void GameRenderer::updateBillboardUBO(Camera const & camera, 
                                      prt::vector<glm::vec4> const & billboardPositions,
                                      prt::vector<glm::vec4> const & billboardColors) {
    auto uboData = getUniformBufferData(graphicsPipelines[billboardPipelineIndex].uboIndex).uboData.data();
    BillboardUBO & ubo = *reinterpret_cast<BillboardUBO*>(uboData);

    ubo.view = camera.getViewMatrix();

    ubo.projection = camera.getProjectionMatrix(nearPlane, farPlane);
    ubo.projection[1][1] *= -1;

 
    for (size_t i = 0; i < billboardPositions.size(); ++i) {   
        glm::vec3 const up{0.0f, 1.0f, 0.0f}; 
        glm::vec3 const altUp{0.0f, 0.0f, 1.0f}; 
        glm::mat4 rot = math_util::safeLookAt(camera.getPosition(), billboardPositions[i], up, altUp);

        ubo.up_vectors[i] = rot * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f);
        ubo.right_vectors[i] = rot * glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
    }

    memcpy(ubo.positions, billboardPositions.data(), sizeof(billboardPositions[0]) * billboardPositions.size());
    memcpy(ubo.colors, billboardColors.data(), sizeof(billboardColors[0]) * billboardColors.size());
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

void GameRenderer::loadModels(Model const * models, size_t nModels, 
                              Texture const * textures, size_t nTextures,
                              size_t assetIndex,
                              bool animated,
                              prt::hash_map<int32_t, int32_t> & textureIndices) {
    // assert(nModels != 0 && "models can not be empty!");

    createVertexBuffer(models, nModels, assetIndex, animated);
    createIndexBuffer(models, nModels, assetIndex);

    Assets & asset = getAssets(assetIndex);
    prt::vector<bool> loaded;
    loaded.resize(nTextures);

    size_t numTex = 0;
    asset.textureImages.images.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageViews.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.imageMemories.resize(NUMBER_SUPPORTED_TEXTURES);
    asset.textureImages.descriptorImageInfos.resize(NUMBER_SUPPORTED_TEXTURES);

    textureIndices.insert(-1, -1);

    for (size_t i = 0; i < nModels; ++i) {
        for (auto const & material: models[i].materials) {
            prt::array<int32_t, 3> indices = { material.albedoIndex,
                                               material.normalIndex,
                                               material.specularIndex };
            for (int32_t ind : indices) {
                if (ind != -1 && !loaded[ind]) {
                    loaded[ind] = true;
                    Texture const & texture = textures[ind];
                    createTextureImage(asset.textureImages.images[numTex], 
                                    asset.textureImages.imageMemories[numTex], 
                                    texture);
                    createTextureImageView(asset.textureImages.imageViews[numTex], 
                                        asset.textureImages.images[numTex], 
                                        texture.mipLevels);

                    textureIndices.insert(ind, numTex);
                    ++numTex;
                }
            } 
        }
    }

    for (size_t i = numTex; i < asset.textureImages.images.size(); ++i) {
        createTextureImage(asset.textureImages.images[i], 
                           asset.textureImages.imageMemories[i], 
                           *Texture::defaultTexture());
        createTextureImageView(asset.textureImages.imageViews[i], 
                               asset.textureImages.images[i], 
                               Texture::defaultTexture()->mipLevels);
    }
}

void GameRenderer::loadBillboards(Billboard const * billboards, size_t nBillboards, 
                                  Texture const * textures, size_t nTextures,
                                  size_t assetIndex,
                                  prt::hash_map<int32_t, int32_t> & textureIndices) {
    assert(nBillboards != 0 && "billboards can not be empty!");

    Assets & asset = getAssets(assetIndex);

    size_t numTex = 0;

    prt::vector<bool> loaded;
    loaded.resize(nTextures);

    asset.textureImages.images.resize(NUMBER_SUPPORTED_BILLBOARD_TEXTURES);
    asset.textureImages.imageViews.resize(NUMBER_SUPPORTED_BILLBOARD_TEXTURES);
    asset.textureImages.imageMemories.resize(NUMBER_SUPPORTED_BILLBOARD_TEXTURES);
    asset.textureImages.descriptorImageInfos.resize(NUMBER_SUPPORTED_BILLBOARD_TEXTURES);

    textureIndices.insert(-1, -1);

    for (size_t i = 0; i < nBillboards; ++i) {
        int32_t ind = billboards[i].textureIndex;
        if (ind != -1 && !loaded[ind]) {
            loaded[ind] = true;
            Texture const & texture = textures[ind];
            createTextureImage(asset.textureImages.images[numTex], 
                            asset.textureImages.imageMemories[numTex], 
                            texture);
            createTextureImageView(asset.textureImages.imageViews[numTex], 
                                asset.textureImages.images[numTex], 
                                texture.mipLevels);

            textureIndices.insert(ind, numTex);
            ++numTex;
        }
    }

    for (size_t i = numTex; i < asset.textureImages.images.size(); ++i) {
        createTextureImage(asset.textureImages.images[i], 
                           asset.textureImages.imageMemories[i], 
                           *Texture::defaultTexture());
        createTextureImageView(asset.textureImages.imageViews[i], 
                               asset.textureImages.images[i], 
                               Texture::defaultTexture()->mipLevels);
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

void GameRenderer::createGridDrawCalls() {
    GraphicsPipeline & pipeline = graphicsPipelines[gridPipelineIndex];

    DrawCall drawCall;
    drawCall.firstIndex = 0;
    drawCall.indexCount = 6;
    pipeline.drawCalls.push_back(drawCall);
}

void GameRenderer::createSkyboxDrawCalls() {
    GraphicsPipeline & pipeline = graphicsPipelines[skyboxPipelineIndex];
    DrawCall drawCall;
    drawCall.firstIndex = 0;
    drawCall.indexCount = 36;
    pipeline.drawCalls.push_back(drawCall);
}

void GameRenderer::createBillboardDrawCalls(Billboard const * billboards,
                                            size_t nBillboards,
                                            prt::hash_map<int32_t, int32_t> const & textureIndices) {
    GraphicsPipeline & pipeline = graphicsPipelines[billboardPipelineIndex];

    DrawCall drawCall;
    drawCall.firstIndex = 0;
    drawCall.indexCount = 6;
    pipeline.drawCalls.resize(nBillboards);
    for (size_t i = 0; i < nBillboards; ++i) {
        Billboard const & billboard = billboards[i];

        pipeline.drawCalls[i] = drawCall;

        BillboardPushConstants & pc = *reinterpret_cast<BillboardPushConstants*>(pipeline.drawCalls[i].pushConstants.data());
        pc.billboardSize.x = billboard.size.x;
        pc.billboardSize.y = billboard.size.y;
        pc.albedoIndex = textureIndices[billboards->textureIndex];
        pc.positionIndex = i;
        
    }
}

void GameRenderer::createModelDrawCalls(Model    const * models,   size_t nModels,
                                        uint32_t const * modelIDs, size_t nModelIDs,
                                        Model    const * animatedModels,   size_t nAnimatedModels,
                                        uint32_t const * animatedModelIDs, size_t nAnimatedModelIDs,
                                        uint32_t const * boneOffsets,
                                        prt::hash_map<int32_t, int32_t> const & textureIndices,
                                        prt::hash_map<int32_t, int32_t> const & animatedTextureIndices,
                                        prt::vector<DrawCall> & standard,
                                        prt::vector<DrawCall> & transparent,
                                        prt::vector<DrawCall> & animated,
                                        prt::vector<DrawCall> & transparentAnimated,
                                        prt::vector<DrawCall> & water,
                                        prt::vector<DrawCall> & shadow,
                                        prt::vector<DrawCall> & shadowAnimated) {
    /* non-animated */
    prt::vector<uint32_t> indexOffsets = { 0 };
    indexOffsets.resize(nModels);
    for (size_t i = 1; i < nModels; ++i) {
        indexOffsets[i] = indexOffsets[i-1] + models[i-1].indexBuffer.size();
    }

    for (size_t i = 0; i < nModelIDs; ++i) {
        const Model& model = models[modelIDs[i]];

        for (auto const & mesh : model.meshes) {
            auto const & material = model.materials[mesh.materialIndex];
            DrawCall drawCall;
            // find texture indices
            int32_t albedoIndex = textureIndices[material.albedoIndex];
            int32_t normalIndex = textureIndices[material.normalIndex];
            int32_t specularIndex = textureIndices[material.specularIndex];
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

            switch (material.type)
            {
            case Model::Material::Type::standard :
                standard.push_back(drawCall);
                shadow.push_back(drawCall);
                break;
            case Model::Material::Type::transparent :
                transparent.push_back(drawCall);
                break;
            case Model::Material::Type::water :
                water.push_back(drawCall);
                break;
            }
        }
    }
    /* animated */
    indexOffsets = { 0 };
    indexOffsets.resize(nAnimatedModels);
    for (size_t i = 1; i < nModels; ++i) {
        indexOffsets[i] = indexOffsets[i-1] + animatedModels[i-1].indexBuffer.size();
    }

    for (size_t i = 0; i < nAnimatedModelIDs; ++i) {
        const Model& model = animatedModels[animatedModelIDs[i]];

        for (auto const & mesh : model.meshes) {
            auto const & material = model.materials[mesh.materialIndex];
            DrawCall drawCall;
            // find texture indices
            int32_t albedoIndex = animatedTextureIndices[material.albedoIndex];
            int32_t normalIndex = animatedTextureIndices[material.normalIndex];
            int32_t specularIndex = animatedTextureIndices[material.specularIndex];
            // push constants
            StandardPushConstants & pc = *reinterpret_cast<StandardPushConstants*>(drawCall.pushConstants.data());
            pc.modelMatrixIdx = i;
            pc.albedoIndex = albedoIndex;
            pc.normalIndex = normalIndex;
            pc.specularIndex = specularIndex;
            pc.baseColor = material.baseColor;
            pc.baseSpecularity = material.baseSpecularity;
            pc.boneOffset = boneOffsets[i];

            // geometry
            drawCall.firstIndex = indexOffsets[animatedModelIDs[i]] + mesh.startIndex;
            drawCall.indexCount = mesh.numIndices;

            switch (material.type)
            {
            case Model::Material::Type::standard :
                animated.push_back(drawCall);
                shadowAnimated.push_back(drawCall);
                break;
            case Model::Material::Type::transparent :
                transparentAnimated.push_back(drawCall);
                break;
            case Model::Material::Type::water :
                assert(false && "Water material not supported for animated model!");
                water.push_back(drawCall);
                break;
            }
        }
    }
}

void GameRenderer::createCompositionDrawCalls(size_t pipelineIndex) {
    GraphicsPipeline & pipeline = graphicsPipelines[pipelineIndex];
    DrawCall drawCall;
    drawCall.firstIndex = 0;
    drawCall.indexCount = 6;
    pipeline.drawCalls.push_back(drawCall);
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

void GameRenderer::createGridBuffers(size_t assetIndex) {
    prt::array<glm::vec3, 6> vertices;

    vertices[0] = glm::vec3{1.0f, 1.0f, 0.0f};
    vertices[1] = glm::vec3{ -1.0f, -1.0f, 0.0f};
    vertices[2] = glm::vec3{-1.0f,  1.0f, 0.0f};
    vertices[3] = glm::vec3{ -1.0f,  -1.0f, 0.0f};
    vertices[4] = glm::vec3{ 1.0f,  1.0f, 0.0f};
    vertices[5] = glm::vec3{ 1.0f,  -1.0f, 0.0f};

    VertexData & data = getAssets(assetIndex).vertexData;
    createAndMapBuffer(vertices.data(), sizeof(glm::vec3) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    

    prt::vector<uint32_t> indices = { 0, 1, 2,
                                      3, 4, 5};
    
    createAndMapBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       data.indexBuffer, 
                       data.indexBufferMemory);   
}

void GameRenderer::createBillboardBuffers(size_t assetIndex) {
    prt::array<glm::vec4, 4> vertices;

    vertices[0] = glm::vec4{-0.5f, -0.5f, 0.0f, 1.0f};
    vertices[1] = glm::vec4{ 0.5f, -0.5f, 0.0f, 0.0f};
    vertices[2] = glm::vec4{-0.5f,  0.5f, 1.0f, 1.0f};
    vertices[3] = glm::vec4{ 0.5f,  0.5f, 1.0f, 0.0f};

    VertexData & data = getAssets(assetIndex).vertexData;
    createAndMapBuffer(vertices.data(), sizeof(glm::vec4) * vertices.size(),
                       VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                       data.vertexBuffer, 
                       data.vertexBufferMemory);    

    prt::vector<uint32_t> indices = { 0, 2, 3,
                                      3, 1, 0};
    
    createAndMapBuffer(indices.data(), sizeof(uint32_t) * indices.size(),
                       VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                       data.indexBuffer, 
                       data.indexBufferMemory);   
}
