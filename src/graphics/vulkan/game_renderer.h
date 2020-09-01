#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"
#include "ubo.h"

#include "src/graphics/lighting/light.h"
#include "src/graphics/camera/camera.h"

#include "src/container/hash_map.h"

class GameRenderer : public VulkanApplication {
public:
    /**
     * @param width frame width
     * @param heght frame height
     */
    GameRenderer(unsigned int width, unsigned int height);

    ~GameRenderer();

    /**
     * binds a scene to the graphics pipeline
     * @param scene : scene to bind
     */
    void bindAssets(Model const * models, size_t nModels,
                    uint32_t const * modelIDs,
                    size_t nModelIDs,
                    Model const * animatedModels,
                    uint32_t const * boneOffsets,
                    size_t nAnimatedModels,
                    uint32_t const * animatedModelIDs,
                    size_t nAnimatedModelIDs,
                    prt::array<Texture, 6> const & skybox);

    /**
     * updates the scene
     * @param modelMatrices : model matrices
     * @param camera : scene camera
     * @param sun : sun light
     * @param time : simulation time in seconds
     */
    void update(prt::vector<glm::mat4> const & modelMatrices, 
                prt::vector<glm::mat4> const & animatedModelMatrices,
                prt::vector<glm::mat4> const & bones,
                Camera & camera,
                DirLight  const & sun,
                float time);

private:
    float nearPlane = 0.03f;
    float farPlane = 500.0f;
    float cascadeSplitLambda = 0.95f;

    int32_t skyboxPipelineIndex = -1;
    int32_t standardPipelineIndex = -1;
    int32_t animatedStandardPipelineIndex = -1;
    int32_t shadowmapPipelineIndex = -1;
    int32_t animatedShadowmapPipelineIndex = -1;

    VkDescriptorImageInfo samplerInfo;

    void createStandardAndShadowGraphicsPipelines(size_t standardAssetIndex, size_t standardUboIndex,
                                                  size_t shadowmapUboIndex, 
                                                  const char * relativeVert, const char * relativeFrag,
                                                  const char * relativeShadowVert,
                                                  VkVertexInputBindingDescription bindingDescription,
                                                  prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                                  int32_t & standardPipeline, int32_t & shadowPipeline);

    void createSkyboxGraphicsPipeline(size_t assetIndex, size_t uboIndex);
    int32_t createStandardGraphicsPipeline(size_t assetIndex, size_t uboIndex, 
                                           char const * vertexShader, char const * fragmentShader,
                                           VkVertexInputBindingDescription bindingDescription,
                                           prt::vector<VkVertexInputAttributeDescription> const & attributeDescription);
                                          
    int32_t createShadowmapGraphicsPipeline(size_t assetIndex, size_t uboIndex,
                                            char const * vertexShader,
                                            VkVertexInputBindingDescription bindingDescription,
                                            prt::vector<VkVertexInputAttributeDescription> const & attributeDescription);

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    void createVertexBuffer(Model const * models, size_t nModels, size_t assetIndex,
                            bool animated);
    
    void createIndexBuffer(Model const * models, size_t nModels, size_t assetIndex);

    void createCubeMapBuffers(size_t assetIndex);

    void loadModels(Model const * models, size_t nModels, size_t assetIndex,
                    bool animated);

    void loadCubeMap(prt::array<Texture, 6> const & skybox, size_t assetIndex);

    void createSkyboxDrawCalls();
    void createStandardDrawCalls(Model    const * models,   size_t nModels,
                                 uint32_t const * modelIDs, size_t nModelIDs,
                                 void const * additionalPushConstants, size_t additionalPushConstantSize,
                                 size_t pipelineIndex);
    void createShadowDrawCalls(size_t shadowPipelineIndex, size_t pipelineIndex);

    void updateUBOs(prt::vector<glm::mat4> const & nonAnimatedModelMatrices, 
                    prt::vector<glm::mat4> const & animatedModelMatrices,
                    prt::vector<glm::mat4> const & bones,
                    Camera & camera,
                    DirLight const & sun,
                    float time);
    void updateSkyboxUBO(Camera const & camera);

    void updateCascades(glm::mat4 const & projectionMatrix,
                        glm::mat4 const & viewMatrix,
                        glm::vec3 const & lightDir,
                        prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> & cascadeSpace,
                        prt::array<float, NUMBER_SHADOWMAP_CASCADES> & splitDepths);
};

#endif
