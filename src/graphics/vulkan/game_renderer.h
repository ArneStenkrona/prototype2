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
    alignas(4)  float t = 0;
    // unsigned char pad12[12];
};

struct LightUBO {
    alignas(4)  float ambientLight;
    alignas(4)  int32_t noPointLights;
    alignas(16) DirLight sun;
    alignas(16) glm::vec4 splitDepths[(NUMBER_SHADOWMAP_CASCADES + 4)/ 4];
    alignas(16) glm::mat4 cascadeSpace[NUMBER_SHADOWMAP_CASCADES];
    alignas(16) PointLight pointLights[NUMBER_SUPPORTED_POINTLIGHTS];
};

struct StandardUBO {
    ModelUBO model;
    LightUBO lighting;
};

struct StandardPushConstants {
    alignas(4)  int32_t modelMatrixIdx;
	alignas(4)  int32_t albedoIndex;
	alignas(4)  int32_t normalIndex;
	alignas(4)  int32_t specularIndex;
    alignas(16) glm::vec3 baseColor;
    alignas(4)  float baseSpecularity;
};

struct SkyboxUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 model;
		//alignas(4) float lodBias = 0.0f;
};

struct ShadowMapUBO  {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 depthVP[NUMBER_SHADOWMAP_CASCADES];
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
     * @param camera : scene camera
     * @param sun : sun light
     * @param time : simulation time in seconds
     */
    void update(prt::vector<glm::mat4> const & modelMatrices, 
                Camera & camera,
                DirLight  const & sun,
                float time);

private:
    float nearPlane = 0.3f;
    float farPlane = 100.0f;
    float cascadeSplitLambda = 0.95f;

    size_t skyboxPipelineIndex;
    size_t standardPipelineIndex;
    size_t shadowmapPipelineIndex;

    VkDescriptorImageInfo samplerInfo;

    void createGraphicsPipelines(size_t skyboxAssetIndex, size_t skyboxUboIndex, 
                                 size_t standardAssetIndex, size_t standardUboIndex, 
                                 size_t shadowmapAssetIndex, size_t shadowmapUboIndex);

    void createSkyboxGraphicsPipeline(size_t assetIndex, size_t uboIndex);
    void createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex, 
                                        const char* vertexShader, const char* fragmentShader);
    void createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                         const char* vertexShader, const char* fragmentShader);

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    // void createVertexBuffer(const prt::vector<Model>& models, size_t assetIndex);
    void createVertexBuffer(Model const * models, size_t nModels, size_t assetIndex);
    
    // void createIndexBuffer(const prt::vector<Model>& models, size_t assetIndex);
    void createIndexBuffer(Model const * models, size_t nModels, size_t assetIndex);

    void createCubeMapBuffers(size_t assetIndex);

    // void loadModels(const prt::vector<Model>& models, size_t assetIndex);
    void loadModels(Model const * models, size_t nModels, size_t assetIndex);

    void loadCubeMap(const prt::array<Texture, 6>& skybox, size_t assetIndex);

    // void createDrawCalls(const prt::vector<Model>& models, const prt::vector<uint32_t>& modelIndices);
    void createDrawCalls(Model const * models, size_t nModels,
                         uint32_t const * modelIDs, size_t nModelIDs);

    void updateUBOs(prt::vector<glm::mat4> const & modelMatrices, 
                    Camera & camera,
                    DirLight  const & sun,
                    float time);
    void updateCascades(glm::mat4 const & projectionMatrix,
                        glm::mat4 const & viewMatrix,
                        glm::vec3 const & lightDir,
                        prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> & cascadeSpace,
                        prt::array<float, NUMBER_SHADOWMAP_CASCADES> & splitDepths);
};

#endif
