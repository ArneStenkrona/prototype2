#ifndef PRT_ASSETS_H
#define PRT_ASSETS_H

#include  "vulkan/vulkan.h"

#include "src/container/vector.h"

#include <cstdlib>

struct VertexData {
    VkBuffer       vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer       indexBuffer;
    VkDeviceMemory indexBufferMemory;
};

struct TextureImages {
    prt::vector<VkImage>               images;
    prt::vector<VkDeviceMemory>        imageMemories;
    prt::vector<VkImageView>           imageViews;
    prt::vector<VkDescriptorImageInfo> descriptorImageInfos;
    // number of loaded textures that are not default
    // textures
    unsigned int                       numTextures;

    void resize(size_t n) { images.resize(n);
                            imageMemories.resize(n);
                            imageViews.resize(n);
                            descriptorImageInfos.resize(n); }
};

struct Assets {
    VertexData vertexData;
    TextureImages textureImages;
};

struct DynamicVertexData {
    VkBuffer          vertexBuffer;
    VkDeviceMemory    vertexBufferMemory;
    VkDeviceSize      vertexBufferSize;
    VkBuffer          indexBuffer;
    VkDeviceMemory    indexBufferMemory;
    VkDeviceSize      indexBufferSize;
    prt::vector<char> vertexData{prt::getAlignment(alignof(std::max_align_t))};
    prt::vector<char> indexData{prt::getAlignment(alignof(std::max_align_t))};
    bool              updated;
    // prt::vector<void*> mappedMemories;
};

struct DynamicAssets {
    prt::vector<DynamicVertexData> vertexData;
    prt::vector<TextureImages>     textureImages;
};

#endif
