#ifndef GRAPHICS_PIPELINE_H
#define GRAPHICS_PIPELINE_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"
#include "src/container/array.h"
    
struct ShaderStage {
    VkShaderStageFlagBits stage;
    char shader[512];
    char pName[512];
};

struct GraphicsPipeline {
    // Assets handle
    size_t assetsIndex;
    // UBO handle
    size_t uboIndex;

    // Descriptors
    prt::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    VkDescriptorPool descriptorPool;
    prt::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    VkDescriptorSetLayout descriptorSetLayout;
    prt::vector<VkDescriptorSet> descriptorSets;
    prt::array<VkDescriptorBufferInfo, 3> descriptorBufferInfos;
    // prt::vector<VkDescriptorImageInfo> descriptorImageInfos;
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

    // Draw calls
    // prt::vector<DrawCall> drawCalls;
};

#endif