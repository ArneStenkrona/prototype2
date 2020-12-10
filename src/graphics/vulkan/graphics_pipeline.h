#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"
#include "src/container/array.h"

struct DrawCall {
    uint32_t firstIndex;
    uint32_t indexCount;
    using PushConstants = prt::array<unsigned char, 64>;
    alignas(16) PushConstants pushConstants;
};

struct ShaderStage {
    VkShaderStageFlagBits stage;
    char shader[512];
    char pName[512];
};

enum PipelineType {
    PIPELINE_TYPE_OPAQUE,
    PIPELINE_TYPE_OFFSCREEN,
    PIPELINE_TYPE_TRANSPARENT,
    PIPELINE_TYPE_COMPOSITION
};

struct GraphicsPipeline {
    static constexpr size_t NULL_INDEX = -1;
    // Assets handle
    size_t assetsIndex;
    // UBO handle
    size_t uboIndex = NULL_INDEX;

    // Descriptors
    prt::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    VkDescriptorPool descriptorPool;
    prt::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    VkDescriptorSetLayout descriptorSetLayout;
    prt::vector<VkDescriptorSet> descriptorSets;
    prt::array<VkDescriptorBufferInfo, 3> descriptorBufferInfos;
    prt::vector<prt::vector<VkWriteDescriptorSet> > descriptorWrites;

    // Pipeline
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    VkRenderPass renderpass;
    VkExtent2D extent;
    VkVertexInputBindingDescription vertexInputBinding;
    prt::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
    prt::vector<ShaderStage> shaderStages;
    bool useColorAttachment;
    bool enableDepthBias;
    unsigned int subpass;

    PipelineType type;
    unsigned int renderGroup = 0;

    // Draw calls
    prt::vector<DrawCall> drawCalls;
};

#endif