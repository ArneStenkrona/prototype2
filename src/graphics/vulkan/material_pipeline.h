#ifndef MATERIAL_PIPELINE_H
#define MATERIAL_PIPELINE_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"
#include "src/container/array.h"

struct DrawCall {
    uint32_t firstIndex;
    uint32_t indexCount;
    // typedef prt::array<uint32_t, 2> ConstantType;
    using PushConstants = prt::array<unsigned char, 32>;
    PushConstants pushConstants;
};
    
struct ShaderStage {
    VkShaderStageFlagBits stage;
    char shader[512];
    char pName[512];
};

struct MaterialPipeline {
    // Assets handle
    size_t assetsIndex;

    // Descriptors
    prt::vector<VkDescriptorSetLayoutBinding> descriptorSetLayoutBindings;
    VkDescriptorPool descriptorPool;
    prt::vector<VkDescriptorPoolSize> descriptorPoolSizes;
    VkDescriptorSetLayout descriptorSetLayout;
    prt::vector<VkDescriptorSet> descriptorSets;
    prt::array<VkDescriptorBufferInfo, 3> descriptorBufferInfos;
    prt::vector<VkDescriptorImageInfo> descriptorImageInfos;
    prt::array<prt::vector<VkWriteDescriptorSet>, 3> descriptorWrites;

    // Pipeline
    VkPipeline pipeline;
    VkPipelineLayout pipelineLayout;
    VkPipelineCache pipelineCache;
    VkVertexInputBindingDescription vertexInputBinding;
    prt::vector<VkVertexInputAttributeDescription> vertexInputAttributes;
    prt::vector<ShaderStage> shaderStages;

    // Draw calls
    prt::vector<DrawCall> drawCalls;
};

#endif