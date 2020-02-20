#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"

#include "src/container/hash_map.h"

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
    prt::vector<size_t> meshMaterialPipelineIndices;
    prt::hash_map<std::string, size_t> shaderToIndex;

    VkDescriptorImageInfo samplerInfo;

    void createMaterialPipelines(prt::vector<Model> const & models);
    void createSkyboxMaterialPipeline(size_t assetIndex);
    void createMeshMaterialPipeline(size_t assetIndex, const char* vertexShader, const char* fragmentShader);

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    void createVertexBuffer(const prt::vector<Model>& models);
    
    void createIndexBuffer(const prt::vector<Model>& models);

    void createSkyboxBuffers();

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