#ifndef MODEL_H
#define MODEL_H

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/container/vector.h"
#include "src/container/array.h"

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    
    /**
     * @return vulkan binding description
     */
    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription = {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
        
        return bindingDescription;
    }
    
    /**
     * @return vulkan attribute description
     */
    static prt::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        prt::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
        
        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);
        
        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, normal);
        
        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);
        
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && normal == other.normal && texCoord == other.texCoord;
    }

    bool operator!=(const Vertex& other) const {
        return !(*this == other);
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.normal) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}

struct Texture {
    prt::vector<unsigned char> pixelBuffer;
    int texWidth, texHeight, texChannels;
    uint32_t mipLevels;

    void load(const char* texturePath);

    inline unsigned char* sample(float x, float y) {
        int sx = static_cast<int>(float(texWidth - 1) * x + 0.5f);
        int sy = static_cast<int>(float(texHeight - 1) * y + 0.5f);
        int si = texChannels * (sy * texWidth + sx);
        return &pixelBuffer[si];
    }
};

struct Material {
    char name[256];
    char fragmentShader[256];
};

struct Mesh {
    size_t startIndex;
    size_t numIndices;
    //char materialName[256];
    Material material; 
    Texture texture;
    char name[256];
};



struct Model {
    prt::vector<Mesh> meshes;
    prt::vector<Vertex> vertexBuffer;
    prt::vector<uint32_t> indexBuffer;
    // char materialFilePath[256];
    void loadOBJ(const char* path);
};

#endif