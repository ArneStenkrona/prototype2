#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"

struct ModelUBO {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 invTransposeModel[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
    alignas(4) float t = 0;
};

struct SkyboxUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 model;
		//alignas(4) float lodBias = 0.0f;
};

class GameRenderer : public VulkanApplication {
public:
    GameRenderer();

    ~GameRenderer();

    void bindScene(Scene const & scene);

    void update(prt::vector<glm::mat4> const & modelMatrices, 
                glm::mat4 const & viewMatrix, 
                glm::mat4 const & projectionMatrix, 
                glm::vec3 const & viewPosition,
                glm::mat4 const & skyProjectionMatrix,
                float time);

private:
    size_t skyboxPipelineIndex;
    size_t modelPipelineIndex;

    VkDescriptorImageInfo samplerInfo;
    // // Model textures
    // prt::array<VkImage, NUMBER_SUPPORTED_TEXTURES> textureImage;
    // prt::array<VkDeviceMemory, NUMBER_SUPPORTED_TEXTURES> textureImageMemory;
    // prt::array<VkImageView, NUMBER_SUPPORTED_TEXTURES> textureImageView;

    // // Cubemap texture
    // VkImage cubeMapImage;
    // VkDeviceMemory cubeMapImageMemory;
    // VkImageView cubeMapImageView;

    void createMaterialPipelines();
    void createSkyboxMaterialPipeline();
    void createModelMaterialPipeline();

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    void createVertexBuffer(const prt::vector<Model>& models);
    
    void createIndexBuffer(const prt::vector<Model>& models);

    void createSkyboxBuffers();

    // void createTextureImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const Texture& texture);
    // void createCubeMapImage(VkImage& texImage, VkDeviceMemory& texImageMemory, const prt::array<Texture, 6>& textures);
    
    // void createTextureImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);
    // void createCubeMapImageView(VkImageView& imageView, VkImage &image, uint32_t mipLevels);

    void loadModels(const prt::vector<Model>& models);

    void loadSkybox(const prt::array<Texture, 6>& skybox);

    void createDrawCalls(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices);

    void updateUBOs(prt::vector<glm::mat4> const & modelMatrices, 
                    glm::mat4 const & viewMatrix, 
                    glm::mat4 const & projectionMatrix, 
                    glm::vec3 const & viewPosition,
                    glm::mat4 const & skyProjectionMatrix,
                    float time);
};

#endif