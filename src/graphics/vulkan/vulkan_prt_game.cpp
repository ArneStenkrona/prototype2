#include "vulkan_prt_game.h"
#ifdef IGNORE_THIS

VulkanPRTGame::VulkanPRTGame(VulkanApplication& vulkanApplication)
    : _vulkanApplication(vulkanApplication) {}

VulkanPRTGame::~VulkanPRTGame() {
    vkDestroyBuffer(_vulkanApplication.device, vertexBuffer, nullptr);
    vkFreeMemory(_vulkanApplication.device, vertexBufferMemory, nullptr);

    vkDestroyBuffer(_vulkanApplication.device, indexBuffer, nullptr);
    vkFreeMemory(_vulkanApplication.device, indexBufferMemory, nullptr);

    vkDestroyBuffer(_vulkanApplication.device, indirectCommandBuffer, nullptr);
    vkFreeMemory(_vulkanApplication.device, indirectCommandBufferMemory, nullptr);
}

void VulkanPRTGame::loadModels(prt::vector<Model>& models) {
    assert(!models.empty());
    for (size_t i = 0; i < pushConstants.size(); i++) {
        pushConstants[i] = i;
    }
    createVertexBuffer(models);
    createIndexBuffer(models);
    assert((models.size() < NUMBER_SUPPORTED_TEXTURES));

    for (size_t i = 0; i < models.size(); i++) {
        createTextureImage(i, models[i]._texture);
        createTextureImageView(i, textureImage[i]);
    }

    for (size_t i = models.size(); i < NUMBER_SUPPORTED_TEXTURES; i++) {
        createTextureImage(i, models[0]._texture);
        createTextureImageView(i, textureImage[0]);
    }

    createIndirectCommandBuffer(models);
    recreateSwapChain();
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

#endif