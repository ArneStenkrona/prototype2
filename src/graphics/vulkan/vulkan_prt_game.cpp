#include "vulkan_prt_game.h"
#ifdef IGNORE_THIS

VulkanPRTGame::VulkanPRTGame(VkDevice& device)
    : _device(device) {}

VulkanPRTGame::~VulkanPRTGame() {
    for (size_t i = 0; i < NUMBER_SUPPORTED_TEXTURES; i++) {
        vkDestroyImageView(_device, textureImageView[i], nullptr);
        vkDestroyImage(_device, textureImage[i], nullptr);
        vkFreeMemory(_device, textureImageMemory[i], nullptr);
    }

    vkDestroyBuffer(_device, vertexBuffer, nullptr);
    vkFreeMemory(_device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(_device, indexBuffer, nullptr);
    vkFreeMemory(_device, indexBufferMemory, nullptr);

    vkDestroyBuffer(_device, indirectCommandBuffer, nullptr);
    vkFreeMemory(_device, indirectCommandBufferMemory, nullptr);
}

void VulkanPRTGame::loadModels(prt::vector<Model>& models) {
    assert(!models.empty());
    for (size_t i = 0; i < _vulkanApplication.pushConstants.size(); i++) {
        _vulkanApplication.pushConstants[i] = i;
    }
    createVertexBuffer(models);
    createIndexBuffer(models);
    assert((models.size() < NUMBER_SUPPORTED_TEXTURES));

    for (size_t i = 0; i < models.size(); i++) {
        _vulkanApplication.createTextureImage(textureImage[i], textureImageMemory[i], models[i]._texture);
        _vulkanApplication.createTextureImageView(textureImageView[i], textureImage[i]);
    }

    for (size_t i = models.size(); i < NUMBER_SUPPORTED_TEXTURES; i++) {
        _vulkanApplication.createTextureImage(textureImage[i], textureImageMemory[i], models[0]._texture);
        _vulkanApplication.createTextureImageView(textureImageView[i], textureImage[0]);
    }

    createIndirectCommandBuffer(models);
    _vulkanApplication.recreateSwapChain();
}

void VulkanPRTGame::createVertexBuffer(prt::vector<Model>& models) {
    prt::vector<Vertex> allVertices;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._vertexBuffer.size(); j++) {
            allVertices.push_back(models[i]._vertexBuffer[j]);
        }
    }
    _vulkanApplication.createAndMapBuffer(allVertices.data(), sizeof(Vertex) * allVertices.size(),
                                          VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                                          vertexBuffer, vertexBufferMemory);    
}

void VulkanPRTGame::createIndexBuffer(prt::vector<Model>& models) {
    prt::vector<uint32_t> allIndices;
    size_t indexOffset = 0;
    for (size_t i = 0; i < models.size(); i++) {
        for (size_t j = 0; j < models[i]._indexBuffer.size(); j++) {
            allIndices.push_back(models[i]._indexBuffer[j] + indexOffset);
        }
        indexOffset += models[i]._vertexBuffer.size();
    }

    _vulkanApplication.createAndMapBuffer(allIndices.data(), sizeof(uint32_t) * allIndices.size(),
                                         VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                                         indexBuffer, indexBufferMemory);
}

void VulkanPRTGame::createIndirectCommandBuffer(prt::vector<Model>& models) {
    prt::vector<VkDrawIndexedIndirectCommand> indirectCommands;
    size_t indexOffset = 0;
    for (size_t i = 0; i < models.size(); i++) {
        VkDrawIndexedIndirectCommand indirectCmd{};
		indirectCmd.instanceCount = 1;
		indirectCmd.firstInstance = i;
		indirectCmd.firstIndex = indexOffset;
		indirectCmd.indexCount = models[i]._indexBuffer.size();

        indirectCommands.push_back(indirectCmd);

        indexOffset += models[i]._indexBuffer.size();
    }

    _vulkanApplication.createAndMapBuffer(indirectCommands.data(), sizeof(VkDrawIndexedIndirectCommand) * indirectCommands.size(),
                                         VK_BUFFER_USAGE_INDIRECT_BUFFER_BIT,
                                         indirectCommandBuffer, indirectCommandBufferMemory);
}

void VulkanPRTGame::createUniformBuffers() {
    VkDeviceSize bufferSize = sizeof(MVPUniformBufferObject);
    
    uniformBuffers.resize(_vulkanApplication.swapChainImages.size());
    uniformBuffersMemory.resize(_vulkanApplication.swapChainImages.size());
    
    for (size_t i = 0; i < _vulkanApplication.swapChainImages.size(); i++) {
        _vulkanApplication.createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);
    }
}

/*void VulkanPRTGame::createDescriptorPool() {
    prt::array<VkDescriptorPoolSize, 3> poolSizes = {};
    poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    poolSizes[0].descriptorCount = static_cast<uint32_t>(_vulkanApplication.swapChainImages.size());
    poolSizes[1].type = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
    poolSizes[1].descriptorCount = static_cast<uint32_t>(NUMBER_SUPPORTED_TEXTURES * _vulkanApplication.swapChainImages.size());
    poolSizes[2].type = VK_DESCRIPTOR_TYPE_SAMPLER;
    poolSizes[2].descriptorCount = static_cast<uint32_t>(swapChainImages.size());

    VkDescriptorPoolCreateInfo poolInfo = {};
    poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
    poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
    poolInfo.pPoolSizes = poolSizes.data();
    poolInfo.maxSets = static_cast<uint32_t>(swapChainImages.size());
    
    if (vkCreateDescriptorPool(_vulkanApplication.device, &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
        throw std::runtime_error("failed to create descriptor pool!");
    }
}*/

/*void VulkanPRTGame::createDescriptorSets() {
    prt::vector<VkDescriptorSetLayout> layouts(_vulkanApplication.swapChainImages.size(), _vulkanApplication.descriptorSetLayout);
    VkDescriptorSetAllocateInfo allocInfo = {};
    allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
    allocInfo.descriptorPool = descriptorPool;
    allocInfo.descriptorSetCount = static_cast<uint32_t>(_vulkanApplication.swapChainImages.size());
    allocInfo.pSetLayouts = layouts.data();
    
    descriptorSets.resize(_vulkanApplication.swapChainImages.size());
    if (vkAllocateDescriptorSets(_vulkanApplication.device, &allocInfo, descriptorSets.data()) != VK_SUCCESS) {
        throw std::runtime_error("failed to allocate descriptor sets!");
    }
    
    for (size_t i = 0; i < swapChainImages.size(); i++) {
        VkDescriptorBufferInfo bufferInfo = {};
        bufferInfo.buffer = uniformBuffers[i];
        bufferInfo.offset = 0;
        bufferInfo.range = sizeof(UniformBufferObject);
        
        prt::array<VkDescriptorImageInfo, NUMBER_SUPPORTED_TEXTURES> imageInfos;
        for (size_t j = 0; j < NUMBER_SUPPORTED_TEXTURES; j++) {
            imageInfos[j].sampler = sampler;
            imageInfos[j].imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfos[j].imageView = textureImageView[j];
        }
        
        prt::array<VkWriteDescriptorSet, 3> descriptorWrites = {};
        
        descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[0].dstSet = descriptorSets[i];
        descriptorWrites[0].dstBinding = 0;
        descriptorWrites[0].dstArrayElement = 0;
        descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        descriptorWrites[0].descriptorCount = 1;
        descriptorWrites[0].pBufferInfo = &bufferInfo;

        descriptorWrites[1] = {};
        descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[1].dstBinding = 1;
        descriptorWrites[1].dstArrayElement = 0;
        descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
        descriptorWrites[1].descriptorCount = NUMBER_SUPPORTED_TEXTURES;
        descriptorWrites[1].dstSet = descriptorSets[i];
        descriptorWrites[1].pBufferInfo = 0;
        descriptorWrites[1].pImageInfo = imageInfos.data();

        VkDescriptorImageInfo samplerInfo = {};
	    samplerInfo.sampler = sampler;

        descriptorWrites[2] = {};
        descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        descriptorWrites[2].dstBinding = 2;
        descriptorWrites[2].dstArrayElement = 0;
        descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
        descriptorWrites[2].descriptorCount = 1;
        descriptorWrites[2].dstSet = descriptorSets[i];
        descriptorWrites[2].pBufferInfo = 0;
        descriptorWrites[2].pImageInfo = &samplerInfo;

        vkUpdateDescriptorSets(device, static_cast<uint32_t>(descriptorWrites.size()), descriptorWrites.data(), 0, nullptr);
    }
}*/

void VulkanPRTGame::updateUniformBuffer(uint32_t currentImage, const prt::vector<glm::mat4>& modelMatrices, glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition) {    
    MVPUniformBufferObject ubo = {};
    for (size_t i = 0; i < modelMatrices.size(); i++) {
        ubo.model[i] = modelMatrices[i];
    }
    ubo.view = viewMatrix;
    ubo.proj = projectionMatrix;
    ubo.proj[1][1] *= -1;
    ubo.viewPosition = viewPosition;
    
    void* data;
    vkMapMemory(_vulkanApplication.device, uniformBuffersMemory[currentImage], 0, sizeof(ubo), 0, &data);
    memcpy(data, &ubo, sizeof(ubo));
    vkUnmapMemory(_vulkanApplication.device, uniformBuffersMemory[currentImage]);
}

void VulkanPRTGame::issueCommands(VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet) {
    if (_modelIDs.empty()) { 
        return;
    }
    VkPipelineLayout& pipelineLayout = _vulkanApplication.pipelineLayout;// Might want to rethink exposing the pipeline in this manner
    VkBuffer vertexBuffers[] = {vertexBuffer};
    VkDeviceSize offsets[] = {0};
    vkCmdBindVertexBuffers(commandBuffer, 0, 1, &vertexBuffers[0], &offsets[0]);
    
    vkCmdBindIndexBuffer(commandBuffer, indexBuffer, 0, VK_INDEX_TYPE_UINT32);
    
    vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipelineLayout, 0, 1, &descriptorSet, 0, nullptr);

    // Issue indirect commands
    for (size_t j = 0; j < _modelIDs.size(); j++)
    {
        vkCmdPushConstants(commandBuffer, pipelineLayout, 
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, 0, sizeof(uint32_t), (void *)&j );
        vkCmdPushConstants(commandBuffer, pipelineLayout, 
                            VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(uint32_t)/*offset*/, sizeof(uint32_t), (void *)&_modelIDs[j] );
        vkCmdDrawIndexedIndirect(commandBuffer, indirectCommandBuffer, 
                                    _modelIDs[j] * sizeof(VkDrawIndexedIndirectCommand), 
                                    1, sizeof(VkDrawIndexedIndirectCommand));
    }
}

#endif