#ifndef PRT_MODEL_H
#define PRT_MODEL_H

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/container/vector.h"
#include "src/container/array.h"
#include "src/container/hash_map.h"
#include "src/container/hash_set.h"


#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <assimp/scene.h>

struct Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    
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
    static prt::array<VkVertexInputAttributeDescription, 5> getAttributeDescriptions() {
        prt::array<VkVertexInputAttributeDescription, 5> attributeDescriptions = {};
        
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

        attributeDescriptions[3].binding = 0;
        attributeDescriptions[3].location = 3;
        attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[3].offset = offsetof(Vertex, tangent);

        attributeDescriptions[4].binding = 0;
        attributeDescriptions[4].location = 4;
        attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[4].offset = offsetof(Vertex, bitangent);
        
        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos      == other.pos      && 
               normal   == other.normal   && 
               texCoord == other.texCoord &&
               tangent  == other.tangent  &&
               bitangent == other.bitangent;
    }

    bool operator!=(const Vertex& other) const {
        return !(*this == other);
    }
};

struct Texture {
    // char path[256];
    prt::vector<unsigned char> pixelBuffer;
    int texWidth, texHeight, texChannels;
    uint32_t mipLevels;

    void load(char const * path);

    inline unsigned char* sample(float x, float y) {
        int sx = static_cast<int>(float(texWidth - 1) * x + 0.5f);
        int sy = static_cast<int>(float(texHeight - 1) * y + 0.5f);
        int si = texChannels * (sy * texWidth + sx);
        return &pixelBuffer[si];
    }
};

struct Material {
    char name[256];
    // char fragmentShader[256];
    int32_t albedoIndex = -1;
    int32_t normalIndex = -1;
    int32_t specularIndex = -1;
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f};
    float baseSpecularity = 0.5f;
};

struct Mesh {
    size_t startIndex;
    size_t numIndices;
    int32_t materialIndex = 0;
    char name[256];
};

struct Model {
    prt::vector<Mesh> meshes;
    prt::vector<Material> materials;
    prt::vector<Texture> textures;
    prt::vector<Vertex> vertexBuffer;
    prt::vector<uint32_t> indexBuffer;
    // void loadOBJ(const char* path);
    // void loadFBX(const char* path);
    void load(char const * path);

private:
    void calcTangentSpace();
    bool mLoaded = false;
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((((((hash<glm::vec3>()(vertex.pos) ^ 
                        (hash<glm::vec3>()(vertex.normal)   << 1)) >> 1) ^ 
                        (hash<glm::vec2>()(vertex.texCoord) << 1)) >> 1) ^
                        (hash<glm::vec3>()(vertex.tangent)  << 1)) >> 1) ^
                        (hash<glm::vec3>()(vertex.bitangent) << 1);
        }
    };

    // thanks, Basile Starynkevitch!
    template<> struct hash<aiString> {
        size_t operator()(aiString const& str) const {
            static constexpr int A = 54059; /* a prime */
            static constexpr int B = 76963; /* another prime */
            // static constexpr int C = 86969; /* yet another prime */
            static constexpr int FIRSTH = 37; /* also prime */
            unsigned h = FIRSTH;
            char const *s = str.C_Str();
            while (*s) {
                h = (h * A) ^ (s[0] * B);
                s++;
            }
            return h; // or return h % C;
        }
    };
}

#endif