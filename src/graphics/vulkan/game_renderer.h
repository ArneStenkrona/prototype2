#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"

#include "src/graphics/lighting/light.h"

#include "src/container/hash_map.h"

struct ModelUBO {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 invTransposeModel[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
    alignas(4) float t = 0;
    // unsigned char pad12[12];
};

struct LightUBO {
    alignas(4) float ambientLight;
    alignas(4) int32_t noPointLights;
    alignas(16) DirLight sun;
    alignas(16) PointLight pointLights[NUMBER_SUPPORTED_POINTLIGHTS];
};

struct StandardUBO {
    ModelUBO model;
    LightUBO lighting;
};

struct SkyboxUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 model;
		//alignas(4) float lodBias = 0.0f;
};

struct ShadowMapUBO  {
    alignas(16) glm::mat4 depthMVP;
};

class GameRenderer : public VulkanApplication {
public:
    GameRenderer(unsigned int width, unsigned int height);

    ~GameRenderer();

    /**
     * binds a scene to the graphics pipeline
     * @param scene : scene to bind
     */
    void bindScene(Scene const & scene);

    /**
     * updates the scene
     * @param modelMatrices : model matrices
     * @param viewMatrix : view matrix
     * @param projectionMatrix : projection matrix 
     * @param viewPosition : view position
     * @param skyProjectionMatrix : projection matrix for skybox
     * @param sun : sun light
     * @param time : simulation time in seconds
     */
    void update(prt::vector<glm::mat4> const & modelMatrices, 
                glm::mat4 const & viewMatrix, 
                glm::mat4 const & projectionMatrix, 
                glm::vec3 const & viewPosition,
                glm::mat4 const & skyProjectionMatrix,
                DirLight  const & sun,
                float time);

private:
    size_t skyboxPipelineIndex;
    size_t standardPipelineIndex;
    size_t shadowmapPipelineIndex;

    VkDescriptorImageInfo samplerInfo;

    void createGraphicsPipelines();
    void createSkyboxGraphicsPipeline(size_t assetIndex, size_t uboIndex);
    void createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex, 
                                        const char* vertexShader, const char* fragmentShader);
    void createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                         const char* vertexShader, const char* fragmentShader);

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
                    DirLight  const & sun,
                    float time);
};

#endif
