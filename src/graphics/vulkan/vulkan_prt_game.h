#ifndef VULKAN_PRT_GAME_H
#define VULKAN_PRT_GAME_H

#include "vulkan_application.h"

#include "src/graphics/geometry/model.h"

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>


/*struct UniformBufferObject {
    alignas(16) glm::mat4 model[MAXIMUM_MODEL_ENTITIES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
};*/

class VulkanPRTGame {
public:
    VulkanPRTGame(VulkanApplication& vulkanApplication);
    ~VulkanPRTGame();

    /*void update(const prt::vector<glm::mat4>& modelMatrices, 
                glm::mat4& viewMatrix, glm::mat4& projectionMatrix, glm::vec3 viewPosition);*/
    void loadModels(prt::vector<Model>& models);
    void bindStaticEntities(const prt::vector<uint32_t>& modelIDs);

private:
    VulkanApplication& _vulkanApplication;

    // Geometry data;
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    // Texture data
    uint32_t mipLevels;
    prt::array<VkImage, NUMBER_SUPPORTED_TEXTURES> textureImage;
    prt::array<VkDeviceMemory, NUMBER_SUPPORTED_TEXTURES> textureImageMemory;
    prt::array<VkImageView, NUMBER_SUPPORTED_TEXTURES> textureImageView;
    // Sampler
    VkSampler sampler;
    // Contains the indirect drawing commands
	VkBuffer indirectCommandBuffer;
    VkDeviceMemory indirectCommandBufferMemory;

    void createTextureImage(size_t index, const Texture& texture);
    void createTextureImageView(size_t index, VkImage &texIm);

    void createVertexBuffer(prt::vector<Model>& models);
    void createIndexBuffer(prt::vector<Model>& models);
    void createIndirectCommandBuffer(prt::vector<Model>& models);

    void drawFrame(const prt::vector<glm::mat4>& modelMatrices, glm::mat4& viewMatrix, 
                   glm::mat4& projectionMatrix, glm::vec3 viewPosition);
    
    void createGraphicsPipeline();

};

#endif