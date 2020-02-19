#ifndef RENDER_PIPELINE_H
#define RENDER_PIPELINE_H
class RenderPipeline {
public:
    RenderPipeline();
    ~RenderPipeline();

    RenderPipeline & operator=(const RenderPipeline&) = delete;
    RenderPipeline(const RenderPipeline&) = delete;

    void createDrawCommands(VkCommandBuffer const & commandBuffer);
private:
        VkBuffer vertexBuffer;
        VkDeviceMemory vertexBufferMemory;
        VkBuffer indexBuffer;
        VkDeviceMemory indexBufferMemory;

        VkPipeline pipeline;
        VkPipelineCache pipelineCache;
        VkPipelineLayout pipelineLayout;

        struct DrawCall {
            uint32_t modelMatrixIndex;
            uint32_t textureIndex;
            uint32_t firstIndex;
            uint32_t indexCount;
        };
        prt::vector<DrawCall> drawCalls;

        struct TextureImages {
            prt::vector<VkImage> images;
            prt::vector<VkDeviceMemory> imageMemories;
            prt::vector<VkImageView> imageViews;
        };
};
#endif