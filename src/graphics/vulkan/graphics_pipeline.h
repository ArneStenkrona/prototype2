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

struct GUIDrawCall {
    uint32_t indexOffset;
    uint32_t indexCount;
    uint32_t vertexOffset;
    VkRect2D scissor;
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
    PIPELINE_TYPE_GUI,
    PIPELINE_TYPE_COMPOSITION
};

struct ImageAttachment {
    size_t descriptorIndex;
    prt::vector<unsigned int> FBAIndices;
    VkImageLayout layout;
    VkSampler sampler;
};


struct UBOAttachment {
    prt::vector<VkDescriptorBufferInfo> descriptorBufferInfos;
    size_t descriptorIndex;
};

struct GraphicsPipeline {
    static constexpr size_t NULL_INDEX = -1;
    // Assets handle
    size_t assetsIndex = NULL_INDEX;
    size_t dynamicAssetsIndex = NULL_INDEX;
    // UBO handle
    size_t uboIndex = NULL_INDEX;

    // Descriptors
    prt::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    VkDescriptorPool descriptorPool;
    prt::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    VkDescriptorSetLayout descriptorSetLayout;
    prt::vector<VkDescriptorSet> descriptorSets;
    prt::vector<prt::vector<VkWriteDescriptorSet> > descriptorWrites;

    VkDescriptorImageInfo textureSamplerInfo;

    // Pipeline
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    size_t renderPassIndex;
    VkExtent2D extent;
    VkVertexInputBindingDescription vertexInputBinding;
    prt::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
    prt::vector<ShaderStage> shaderStages;

    // blending
    prt::vector<VkPipelineColorBlendAttachmentState> colorBlendAttachments;

    VkCullModeFlags cullModeFlags = VK_CULL_MODE_BACK_BIT;

    VkPipelineDepthStencilStateCreateInfo depthStencilState;
    // VkCompareOp compareOp = VK_COMPARE_OP_LESS_OR_EQUAL;
    
    bool useColorAttachment; // Remove?
    bool enableDepthBias;

    float depthBiasConstant = 1.0f;
    float depthBiasClamp = 0.0f;
    float depthBiasSlope = 1.0f;

    unsigned int subpass;

    PipelineType type;
    unsigned int renderGroup = 0;

    int textureDescriptorIndex = -1;
    prt::vector<ImageAttachment> imageAttachments;
    prt::vector<UBOAttachment> uboAttachments;

    // Draw calls
    prt::vector<DrawCall> drawCalls;

    prt::vector<GUIDrawCall> guiDrawCalls;
};

#endif