#ifndef GAME_RENDERER_H
#define GAME_RENDERER_H

#include "vulkan_application.h"
#include "ubo.h"

#include "src/graphics/lighting/light.h"
#include "src/graphics/camera/camera.h"
#include "src/graphics/geometry/billboard.h"

#include "src/graphics/vulkan/imgui_application.h"

#include "src/game/scene/entity.h"

#include "src/container/hash_map.h"

struct RenderResult {
    float mouseDepth;
    glm::vec3 mouseWorldPosition;
    int16_t entityID;
};

class GameRenderer : public VulkanApplication {
public:
    /**
     * @param width frame width
     * @param heght frame height
     */
    GameRenderer(unsigned int width, unsigned int height, Input & input);

    ~GameRenderer();
    /**
     * binds a scene to the graphics pipeline
     */
    void bindAssets(Model const * models, size_t nModels,
                    ModelID const * staticModelIDs, EntityID const * staticEntityIDs,
                    size_t nStaticModelIDs,
                    ModelID const * animatedModelIDs, EntityID const * animatedEntityIDs,
                    uint32_t const * boneOffsets,
                    size_t nAnimatedModelIDs,
                    Billboard const * billboards,
                    size_t nBillboards,
                    Texture const * textures,
                    size_t nTextures,
                    prt::array<Texture, 6> const & skybox);

    /**
     * updates the scene
     * @param modelMatrices : model matrices
     * @param camera : scene camera
     * @param sun : sun light
     */
    RenderResult update(prt::vector<glm::mat4> const & modelMatrices, 
                        prt::vector<glm::mat4> const & animatedModelMatrices,
                        prt::vector<glm::mat4> const & bones,
                        prt::vector<glm::vec4> const & billboardPositions,
                        prt::vector<glm::vec4> const & billboardColors,
                        Camera & camera,
                        SkyLight const & sun,
                        prt::vector<PointLight> const & pointLights,
                        prt::vector<PackedBoxLight> const & boxLights,
                        float t,
                        glm::vec2 mousePosition);

    void render(float deltaTime, uint16_t renderGroupMask);

    static constexpr unsigned int COMMON_RENDER_GROUP = 0;
    static constexpr unsigned int GAME_RENDER_GROUP = 1;
    static constexpr uint16_t GAME_RENDER_MASK = RENDER_GROUP_FLAG_0 | RENDER_GROUP_FLAG_1;
    static constexpr unsigned int EDITOR_RENDER_GROUP = 2;
    static constexpr uint16_t EDITOR_RENDER_MASK = RENDER_GROUP_FLAG_0 | RENDER_GROUP_FLAG_2;

private:
    ImGuiApplication m_imguiApplication;

    static constexpr float depthBiasConstant = 0.0f;//0.01f;//1.25f;
    static constexpr float depthBiasSlope = 0.0f;//0.01f;//1.75f;
    float nearPlane = 0.03f;
    float farPlane = 500.0f;
    float maxShadowDistance = 100.0f;
    float cascadeSplitLambda = 0.85f;

    size_t scenePassIndex;
    size_t shadowmapPassIndex;

    // size_t colorFBAIndex;
    size_t depthFBAIndex;
    prt::vector<size_t> accumulationFBAIndices;
    prt::vector<size_t> revealageFBAIndices;
    prt::vector<size_t> shadowmapFBAIndices;
    prt::vector<size_t> objectFBAIndices;

    size_t objectCopyIndex;
    size_t depthCopyIndex;

    size_t shadowMapIndex;

    int32_t gridPipelineIndex = -1;
    int32_t skyboxPipelineIndex = -1;
    int32_t standardPipelineIndex = -1;
    int32_t animatedStandardPipelineIndex = -1;
    int32_t shadowmapPipelineIndex = -1;
    int32_t animatedShadowmapPipelineIndex = -1;
    int32_t transparentPipelineIndex = -1;
    int32_t animatedTransparentPipelineIndex = -1;
    int32_t waterPipelineIndex = -1;
    int32_t billboardPipelineIndex = -1; // transparent
    int32_t compositionPipelineIndex = -1;
    int32_t guiPipelineIndex = -1;

    VkDescriptorImageInfo samplerInfo;

    void createStandardAndShadowPipelines(size_t standardAssetIndex, size_t standardUboIndex,
                                          size_t shadowmapUboIndex, 
                                          const char * relativeVert, const char * relativeFrag,
                                          const char * relativeTransparentFrag,
                                          const char * relativeShadowVert,
                                          VkVertexInputBindingDescription bindingDescription,
                                          prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                          int32_t & standardPipeline, 
                                          int32_t & transparentPipeline,
                                          int32_t & shadowPipeline);

    void createCompositionPipeline();

    void createGridPipeline(size_t assetIndex, size_t uboIndex);

    void createSkyboxPipeline(size_t assetIndex, size_t uboIndex);

    void createBillboardPipeline(size_t assetIndex, size_t uboIndex);

    int32_t createStandardPipeline(size_t assetIndex, size_t uboIndex, 
                                   char const * vertexShader, char const * fragmentShader,
                                   VkVertexInputBindingDescription bindingDescription,
                                   prt::vector<VkVertexInputAttributeDescription> const & attributeDescription,
                                   bool transparent);
                                          
    int32_t createShadowmapPipeline(size_t assetIndex, size_t uboIndex,
                                    char const * vertexShader,
                                    VkVertexInputBindingDescription bindingDescription,
                                    prt::vector<VkVertexInputAttributeDescription> const & attributeDescription);

    void createCommandBuffers();
    void createCommandBuffer(size_t imageIndex);

    void createVertexBuffers(Model const * models, size_t nModels, 
                             size_t staticAssetIndex, size_t animatedAssetIndex);
    
    void createIndexBuffers(Model const * models, size_t nModels,
                            size_t staticAssetIndex, size_t animatedAssetIndex);

    void createGridBuffers(size_t assetIndex);
    void createCubeMapBuffers(size_t assetIndex);
    void createBillboardBuffers(size_t assetIndex);
    
    void loadModels(Model const * models, size_t nModels, 
                    Texture const * textures, size_t nTextures,
                    size_t staticAssetIndex,
                    size_t animatedAssetIndex,
                    prt::hash_map<int32_t, int32_t> & staticTextureIndices,
                    prt::hash_map<int32_t, int32_t> & animatedTextureIndices);

    void loadBillboards(Billboard const * billboards, size_t nBillboards, 
                        Texture const * textures, size_t nTextures,
                        size_t assetIndex,
                        prt::hash_map<int32_t, int32_t> & textureIndices);

    void loadCubeMap(prt::array<Texture, 6> const & skybox, size_t assetIndex);

    void createGridDrawCalls();
    void createSkyboxDrawCalls();
    void createModelDrawCalls(Model const * models,   size_t nModels,
                              ModelID const * staticModelIDs, EntityID const * staticEntityIDs,
                              size_t nStaticModelIDs,
                              ModelID const * animatedModelIDs, EntityID const * animatedEntityIDs,
                              size_t nAnimatedModelIDs,
                              uint32_t const * boneOffsets,
                              prt::hash_map<int32_t, int32_t> const & staticTextureIndices,
                              prt::hash_map<int32_t, int32_t> const & animatedTextureIndices,
                              prt::vector<DrawCall> & standard,
                              prt::vector<DrawCall> & transparent,
                              prt::vector<DrawCall> & animated,
                              prt::vector<DrawCall> & transparentAnimated,
                              prt::vector<DrawCall> & water,
                              prt::vector<DrawCall> & shadow,
                              prt::vector<DrawCall> & shadowAnimated);

    void createBillboardDrawCalls(Billboard const * billboards, size_t nBillboards, prt::hash_map<int32_t, int32_t> const & textureIndices);

    void createShadowDrawCalls(size_t shadowPipelineIndex, size_t pipelineIndex);

    void createCompositionDrawCalls(size_t pipelineIndex);

    void updateUBOs(prt::vector<glm::mat4> const & nonAnimatedModelMatrices, 
                    prt::vector<glm::mat4> const & animatedModelMatrices,
                    prt::vector<glm::mat4> const & bones,
                    prt::vector<glm::vec4> const & billboardPositions,
                    prt::vector<glm::vec4> const & billboardColors,
                    Camera & camera,
                    SkyLight const & sun,
                    prt::vector<PointLight> const & pointLights,
                    prt::vector<PackedBoxLight> const & boxLights,
                    float t);
                    
    void updateSkyboxUBO(Camera const & camera, SkyLight const & sky);

    void updateBillboardUBO(Camera const & camera, 
                            prt::vector<glm::vec4> const & billboardPositions, 
                            prt::vector<glm::vec4> const & billboardColors);

    void updateCascades(glm::mat4 const & projectionMatrix,
                        glm::mat4 const & viewMatrix,
                        glm::vec3 const & lightDir,
                        prt::array<glm::mat4, NUMBER_SHADOWMAP_CASCADES> & cascadeSpace,
                        prt::array<float, NUMBER_SHADOWMAP_CASCADES> & splitDepths);

    void pushBackObjectFBA();
    void pushBackDepthFBA();
    void pushBackAccumulationFBA();
    void pushBackRevealageFBA();
    void pushBackShadowFBA();

    void pushBackSceneRenderPass();
    void pushBackShadowRenderPass();

    void pushBackSunShadowMap();

    prt::vector<VkPipelineColorBlendAttachmentState> getOpaqueBlendAttachmentState();
    prt::vector<VkPipelineColorBlendAttachmentState> getTransparentBlendAttachmentState();
    prt::vector<VkPipelineColorBlendAttachmentState> getCompositionBlendAttachmentState();
    prt::vector<VkPipelineColorBlendAttachmentState> getShadowBlendAttachmentState();
};

#endif
