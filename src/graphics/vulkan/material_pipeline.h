#ifndef MATERIAL_PIPELINE_H
#define MATERIAL_PIPELINE_H

#include <vulkan/vulkan.h>

#include "src/container/vector.h"
#include "src/container/array.h"


struct VertexData {
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

struct TextureImages {
    prt::vector<VkImage> images;
    prt::vector<VkDeviceMemory> imageMemories;
    prt::vector<VkImageView> imageViews;
};
struct DrawCall {
    uint32_t firstIndex;
    uint32_t indexCount;
    typedef prt::array<uint32_t, 2> ConstantType;
    ConstantType pushConstants;
};

struct MaterialPipeline {
    TextureImages textureImages;

    // Vertex data
    VertexData vertexData;

    // Uniform data
    prt::vector<char> uboData;
    prt::vector<VkBuffer> uniformBuffers;
    prt::vector<VkDeviceMemory> uniformBufferMemories;

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
    char vertexShader[512];
    char fragmentShader[512];

    // Draw calls
    prt::vector<DrawCall> drawCalls;
};

#endif