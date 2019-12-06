#ifndef VULKAN_PRT_GAME_H
#define VULKAN_PRT_GAME_H

#include "vulkan_application.h"

#include "src/graphics/geometry/model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


struct MVPUniformBufferObject {
    alignas(16) glm::mat4 model[MAXIMUM_MODEL_ENTITIES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
};

class VulkanPRTGame {
public:
    VulkanPRTGame(VkDevice& device);
    ~VulkanPRTGame();

    /*void update(const prt::vector<glm::mat4>& modelMatrices, 
                glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition);*/
    void loadModels(prt::vector<Model>& models);
    void bindStaticEntities(const prt::vector<uint32_t>& modelIDs);

private:
    //VulkanApplication& _vulkanApplication;
    VkDevice& _device;

    // Geometry data;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    // Uniform data
    prt::vector<VkBuffer> uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBuffersMemory;
    // Texture data
    uint32_t mipLevels;
    prt::array<VkImage, NUMBER_SUPPORTED_TEXTURES> textureImage;
    prt::array<VkDeviceMemory, NUMBER_SUPPORTED_TEXTURES> textureImageMemory;
    prt::array<VkImageView, NUMBER_SUPPORTED_TEXTURES> textureImageView;
    // Sampler
    VkSampler sampler;
    // Descriptors
    //VkDescriptorPool descriptorPool;
    //prt::vector<VkDescriptorSet> descriptorSets;
     // Entities
    prt::vector<uint32_t> _modelIDs;
    // Contains the indirect drawing commands
	VkBuffer indirectCommandBuffer;
    VkDeviceMemory indirectCommandBufferMemory;
    

    void createTextureImage(size_t index, const Texture& texture);
    void createTextureImageView(size_t index, VkImage &texIm);

    void createVertexBuffer(prt::vector<Model>& models);
    void createIndexBuffer(prt::vector<Model>& models);
    void createIndirectCommandBuffer(prt::vector<Model>& models);
    void createUniformBuffers();
    //void createDescriptorPool();
    //void createDescriptorSets();

    void updateUniformBuffer(uint32_t currentImage, const prt::vector<glm::mat4>& modelMatrices, 
                             glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition);
    
    void createGraphicsPipeline();

    void issueCommands(VkCommandBuffer& commandBuffer, VkDescriptorSet& descriptorSet);

};

#endif