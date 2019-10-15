#ifndef MODEL_H
#define MODEL_H

#include "src/container/vector.h"

#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/hash.hpp>

#include <array>

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
    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 3> attributeDescriptions = {};
        
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

struct Mesh {
    size_t startIndex;
    size_t numIndices;
};

struct Texture {
    prt::vector<unsigned char> pixelBuffer;
    int texWidth, texHeight, texChannels;
};

struct Material {
    /* TODO: Add members */
};

class Model {
public:
    /**
     * Initializes an empty model.
     */
    //Model()
    //: vertexBuffer(4 * sizeof(float)),
    //  indexBuffer(sizeof(uint32_t)),
    //  meshes(sizeof(size_t)) {}

    /**
     * Loads a model from a path.
     * @param path path to .obj file.
     * @param allocator allocator for buffers
     */
    Model(std::string& path);

    void load();
    void free();

    bool meshesAreLoaded() const { return _meshesAreLoaded; }
    bool texturesAreLoaded() const { return _texturesAreLoaded; }

    const prt::vector<Vertex>& vertexBuffer() const { assert(_meshesAreLoaded); return  _vertexBuffer; }
    const prt::vector<uint32_t>& indexBuffer() const { assert(_meshesAreLoaded); return  _indexBuffer; }

    const Texture& texture() const { assert(_texturesAreLoaded); return _texture; }

private:   
    std::string _path;

    prt::vector<Vertex> _vertexBuffer;
    prt::vector<uint32_t> _indexBuffer;

    prt::vector<Mesh> _meshes;
    
    Texture _texture;

    bool _meshesAreLoaded;
    bool _texturesAreLoaded;

    void loadMeshes();
    void loadTextures();

    void freeMeshes();
    void freeTextures();

    //prt::array<Material> materials;
    /*
    VkBuffer vertexBuffer;
    VkDeviceMemory vertexBufferMemory;
    VkBuffer indexBuffer;
    VkDeviceMemory indexBufferMemory;
    */
    //friend class VulkanApplication;
};

#endif