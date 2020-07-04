#ifndef PRT_MODEL_H
#define PRT_MODEL_H

#include "texture.h"

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

struct Model {
    struct Mesh;
    struct Material;
    struct Vertex;
    struct BonedVertex;
    struct BoneData;
    struct Bone;
    struct Animation;
    struct AnimatedVertex;
    struct AnimationKey;
    struct AnimationNode;

    prt::vector<Mesh> meshes;
    prt::vector<Animation> animations;
    prt::vector<Material> materials;
    prt::vector<Texture> textures;
    prt::vector<Vertex> vertexBuffer;
    prt::vector<BoneData> vertexBoneBuffer;
    prt::vector<uint32_t> indexBuffer;
    void load(char const * path, bool loadAnimation = false);

private:
    void calcTangentSpace();
    void getTexture(int32_t &textureIndex, aiMaterial &aiMat, aiTextureType type,
                    prt::hash_map<aiString, size_t> &map, const char * modelPath);

    bool mLoaded = false;
    bool mAnimated = false;
};

struct Model::Material {
    char name[256];
    int32_t albedoIndex = -1;
    int32_t normalIndex = -1;
    int32_t specularIndex = -1;
    glm::vec3 baseColor{1.0f, 1.0f, 1.0f};
    float baseSpecularity = 0.5f;
};

struct Model::Bone {
    glm::mat4 offsetMatrix;
};

struct Model::Mesh {
    size_t startIndex;
    size_t numIndices;
    int32_t materialIndex = 0;
    prt::vector<Bone> bones;
    char name[256];
};

struct Model::AnimationKey {
    glm::vec3 position;
    glm::quat rotation;
    //glm::vec3 scaling;
};

struct Model::AnimationNode {
    prt::vector<AnimationKey> keys;
};

struct Model::Animation {
    float duration;
    double ticksPerSecond;
    prt::vector<AnimationNode> channels;
};

struct Model::BoneData {
    uint32_t boneIDs[4] = { 0, 0, 0, 0 };
    float boneWeights[4] = { 0.0f, 0.0f, 0.0f, 0.0f };
};

struct Model::Vertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    
    /**
     * @return vulkan binding description
     */
    static VkVertexInputBindingDescription getBindingDescription();
    
    /**
     * @return vulkan attribute description
     */
    static prt::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();
};

struct Model::BonedVertex {
    glm::vec3 pos;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 tangent;
    glm::vec3 bitangent;
    BoneData boneData;
    
    /**
     * @return vulkan binding description
     */
    static VkVertexInputBindingDescription getBindingDescription();
    
    /**
     * @return vulkan attribute description
     */
    static prt::vector<VkVertexInputAttributeDescription> getAttributeDescriptions();

};

namespace std {
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