#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"
#include "ubo.h"

#include "src/graphics/lighting/light.h"

#include "src/container/hash_map.h"

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
    float nearPlane = 0.03f;
    float farPlane = 100.0f;
    float cascadeSplitLambda = 0.95f;

    size_t skyboxPipelineIndex;
    size_t standardPipelineIndex;
    size_t animatedStandardPipelineIndex;
    size_t shadowmapPipelineIndex;

    VkDescriptorImageInfo samplerInfo;

    void createGraphicsPipelines(size_t skyboxAssetIndex, size_t skyboxUboIndex, 
                                 size_t standardAssetIndex, size_t standardUboIndex, 
                                 size_t shadowmapAssetIndex, size_t shadowmapUboIndex);

    void createSkyboxGraphicsPipeline(size_t assetIndex, size_t uboIndex);
    void createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex, 
                                        const char* vertexShader, const char* fragmentShader,
                                        prt::vector<VkVertexInputAttributeDescription> const & attributeDescription);
    void createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                         const char* vertexShader);

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    void createVertexBuffer(Model const * models, size_t nModels, size_t assetIndex);
    
    void createIndexBuffer(Model const * models, size_t nModels, size_t assetIndex);

    void createCubeMapBuffers(size_t assetIndex);

    void loadModels(Model const * models, size_t nModels, size_t assetIndex);

    void loadCubeMap(const prt::array<Texture, 6>& skybox, size_t assetIndex);

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
