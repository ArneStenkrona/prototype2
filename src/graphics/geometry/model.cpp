#include "model.h"

#include "src/graphics/geometry/fbx_scene.h"

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/quaternion.hpp>

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>

void Model::load(char const * path, bool loadAnimation) {
    assert(!mLoaded && "Model is already loaded!");
    mAnimated = loadAnimation;

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, 
                                aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    
    aiScene const * scene = importer.ReadFile(path,
                                              aiProcess_CalcTangentSpace         |
                                              aiProcess_Triangulate              |
                                              aiProcess_FlipUVs                  |
                                              aiProcess_FindDegenerates          |
                                              aiProcess_JoinIdenticalVertices    |
                                              aiProcess_RemoveRedundantMaterials |
                                              aiProcess_SortByPType);

    // check if import failed
    if(!scene) {
        std::cout << importer.GetErrorString() << std::endl;
        assert(false && "failed to load file!");
    }
    strcpy(name, strchr(path, '/'));

    // parse materials
    prt::hash_map<aiString, size_t> albedoPathToIndex;
    prt::hash_map<aiString, size_t> normalPathToIndex;
    materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < materials.size(); ++i) {
        aiString matName;
        aiGetMaterialString(scene->mMaterials[i], AI_MATKEY_NAME, &matName);
        strcpy(materials[i].name, matName.C_Str());
 
        aiColor3D color;
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
        materials[i].baseColor = { color.r, color.g, color.b };
        
        getTexture(materials[i].albedoIndex, *scene->mMaterials[i], aiTextureType_DIFFUSE,
                   albedoPathToIndex, path);
        getTexture(materials[i].normalIndex, *scene->mMaterials[i], aiTextureType_NORMALS,
                   normalPathToIndex, path);
    }

    /* Process node hierarchy */
    struct TFormNode {
        aiNode* node;
        aiMatrix4x4 tform;
        int32_t parentIndex;
    };

    memcpy(&mGlobalInverseTransform, &scene->mRootNode->mTransformation, sizeof(glm::mat4));
    // assimp row-major, glm col-major
    mGlobalInverseTransform = glm::transpose(glm::inverse(mGlobalInverseTransform));
    
    prt::hash_map<aiString, size_t> nodeToIndex;
    prt::vector<aiString> boneToName;

    prt::vector<TFormNode> nodes;
    nodes.push_back({scene->mRootNode, scene->mRootNode->mTransformation, -1});
    while(!nodes.empty()) {
        aiNode *node = nodes.back().node;
        int32_t parentIndex = nodes.back().parentIndex;
        aiMatrix4x4 & nodeTform = node->mTransformation;
        aiMatrix4x4 tform = nodes.back().tform;
        aiMatrix3x3 invtpos = aiMatrix3x3(tform);
        invtpos.Inverse().Transpose();
        nodes.pop_back();

        // add node member
        int32_t nodeIndex = mNodes.size();
        mNodes.push_back({});
        Node & n = mNodes.back();
        memcpy(&n.transform, &nodeTform, sizeof(glm::mat4));
        // assimp row-major, glm col-major
        n.transform = glm::transpose(n.transform);
        nodeToIndex.insert(node->mName, nodeIndex);
        
        n.parentIndex = parentIndex;
        if (n.parentIndex != -1) {
           mNodes[n.parentIndex].childIndices.push_back(nodeIndex);
        }

        // process all the node's meshes (if any)
        for(size_t i = 0; i < node->mNumMeshes; ++i) {
            aiMesh *aiMesh = scene->mMeshes[node->mMeshes[i]]; 
            if (aiMesh->mNumFaces == 0) continue; 

            // resize vertex buffer
            size_t prevVertSize = vertexBuffer.size();
            vertexBuffer.resize(prevVertSize + aiMesh->mNumVertices);
            // parse mesh
            meshes.push_back({});
            Mesh &mesh = meshes.back();
            strcpy(mesh.name, aiMesh->mName.C_Str()); 
            mesh.materialIndex = aiMesh->mMaterialIndex;

            size_t vert = prevVertSize;
            bool hasTexCoords = aiMesh->HasTextureCoords(0);
            for (size_t j = 0; j < aiMesh->mNumVertices; ++j) {
                aiVector3D pos = tform * aiMesh->mVertices[j];
                vertexBuffer[vert].pos.x = pos.x;
                vertexBuffer[vert].pos.y = pos.y;
                vertexBuffer[vert].pos.z = pos.z;

                aiVector3D norm = (invtpos * aiMesh->mNormals[j]).Normalize();
                vertexBuffer[vert].normal.x = norm.x;
                vertexBuffer[vert].normal.y = norm.y;
                vertexBuffer[vert].normal.z = norm.z;

                if (hasTexCoords) {
                    vertexBuffer[vert].texCoord.x = aiMesh->mTextureCoords[0][j].x;
                    vertexBuffer[vert].texCoord.y = aiMesh->mTextureCoords[0][j].y;
                }
                ++vert;
            }
            size_t prevIndSize = indexBuffer.size();
            indexBuffer.resize(prevIndSize + 3 * aiMesh->mNumFaces);
            mesh.startIndex = prevIndSize;
            mesh.numIndices = 3 * aiMesh->mNumFaces;
            size_t ind = prevIndSize;
            for (size_t j = 0; j < aiMesh->mNumFaces; ++j) {
                indexBuffer[ind++] = prevVertSize + aiMesh->mFaces[j].mIndices[0];
                indexBuffer[ind++] = prevVertSize + aiMesh->mFaces[j].mIndices[1];
                indexBuffer[ind++] = prevVertSize + aiMesh->mFaces[j].mIndices[2];
            }

            // bones
            if (loadAnimation) {
                vertexBoneBuffer.resize(vertexBuffer.size());
                size_t prevBoneSize = bones.size();
                bones.resize(prevBoneSize + aiMesh->mNumBones);
                for (size_t j = 0; j < aiMesh->mNumBones; ++j) {
                    size_t bi = prevBoneSize + j;
                    aiBone const * bone = aiMesh->mBones[j];
                    boneToName.push_back(bone->mName);
                    memcpy(&bones[bi].offsetMatrix, &bone->mOffsetMatrix, sizeof(glm::mat4));

                    memcpy(&bones[bi].meshTransform, &tform, sizeof(glm::mat4));
                    bones[bi].meshTransform = glm::transpose(bones[bi].meshTransform);
                    // assimp row-major, glm col-major
                    bones[bi].offsetMatrix = glm::transpose(bones[bi].offsetMatrix) * glm::inverse(bones[bi].meshTransform);
                    // store the bone weights and IDs in vertices
                    for (size_t iv = 0; iv < bone->mNumWeights; ++iv) {
                        BoneData & bd = vertexBoneBuffer[prevVertSize + bone->mWeights[iv].mVertexId];
                        // only store the 4 most influential bones
                        int leastInd = -1;
                        float weight = bone->mWeights[iv].mWeight;
                        float leastWeight = weight;
                        for (size_t ind = 0; ind < 4; ++ind) {
                            if (bd.boneWeights[ind] < leastWeight) {
                                leastWeight = bd.boneWeights[ind];
                                leastInd = ind;
                            }
                        }
                        if (leastInd != -1) {
                            bd.boneIDs[leastInd] = bi;
                            bd.boneWeights[leastInd] = weight;
                        }
                    }
                    // make sure bone weights add up to 1 for each boned vertex
                    for (auto & bd : vertexBoneBuffer) {
                        if (glm::length2(bd.boneWeights) > 0) {
                            float boneSum = bd.boneWeights[0] + bd.boneWeights[1] + bd.boneWeights[2] + bd.boneWeights[3];
                            bd.boneWeights = bd.boneWeights / boneSum;                            
                        }
                    } 
                }
            }
        }

        // process all children of the node
        for (size_t i = 0; i < node->mNumChildren; ++i) {
            nodes.push_back({node->mChildren[i], tform * node->mChildren[i]->mTransformation, nodeIndex});
        }
    }
    // parse animations
    if (loadAnimation) {
        animations.resize(scene->mNumAnimations);
        for (size_t i = 0; i < scene->mNumAnimations; ++i) {
            aiAnimation const * aiAnim = scene->mAnimations[i];

            nameToAnimation.insert(aiAnim->mName, i);

            Animation & anim = animations[i];
            anim.duration = aiAnim->mDuration;
            anim.ticksPerSecond = aiAnim->mTicksPerSecond;
            anim.channels.resize(aiAnim->mNumChannels);

            for (size_t j = 0; j < aiAnim->mNumChannels; ++j) {
                aiNodeAnim const * aiChannel = aiAnim->mChannels[j];
                AnimationNode & channel = anim.channels[j];

                assert(nodeToIndex.find(aiChannel->mNodeName) != nodeToIndex.end() && "animation does not correspond to node");
                auto nodeIndex = nodeToIndex.find(aiChannel->mNodeName)->value();
                mNodes[nodeIndex].channelIndex = j;

                assert(aiChannel->mNumPositionKeys == aiChannel->mNumRotationKeys && 
                       aiChannel->mNumPositionKeys == aiChannel->mNumScalingKeys && "number of position, rotation and scaling keys need to match");
                channel.keys.resize(aiChannel->mNumPositionKeys);

                for (size_t k = 0; k < channel.keys.size(); ++k) {
                    aiVector3D const & aiPos = aiChannel->mPositionKeys[k].mValue;
                    aiQuaternion const & aiRot = aiChannel->mRotationKeys[k].mValue;
                    aiVector3D const & aiScale = aiChannel->mScalingKeys[k].mValue;
                    channel.keys[k].position = { aiPos.x, aiPos.y, aiPos.z };
                    channel.keys[k].rotation = { aiRot.w, aiRot.x, aiRot.y, aiRot.z };
                    channel.keys[k].scaling = { aiScale.x, aiScale.y, aiScale.z };
                }
            }
        }

        // set node Indices
        for (size_t i = 0; i < bones.size(); ++i) {
            assert(nodeToIndex.find(boneToName[i]) != nodeToIndex.end() && "No corresponding node for bone");
            size_t nodeIndex = nodeToIndex.find(boneToName[i])->value();
            mNodes[nodeIndex].boneIndex = i;
        }
    }

    // release assimp resources
    mLoaded = true;
    calcTangentSpace();
}

uint32_t Model::getAnimationIndex(char const * name) const {
    assert(nameToAnimation.find(aiString(name)) != nameToAnimation.end() && "animation does not exist");
    return nameToAnimation.find(aiString(name))->value();
}

void Model::sampleAnimation(float t, size_t animationIndex, glm::mat4 * transforms) const {
    assert(mAnimated);
    auto const & animation = animations[animationIndex];

    // calculate prev and next frame
    float duration = animation.duration / (1000 * animation.ticksPerSecond);
    float animTime = t / duration;
    size_t numFrames = animation.channels[0].keys.size();
    float fracFrame = animTime * numFrames;
    uint32_t prevFrame = static_cast<uint32_t>(fracFrame);
    float frac = fracFrame - prevFrame;
    prevFrame = prevFrame % numFrames;
    uint32_t nextFrame = (prevFrame + 1) % numFrames;

    struct IndexedTForm {
        int32_t index;
        glm::mat4 tform;
    };

    prt::vector<IndexedTForm> nodeIndices;
    nodeIndices.push_back({0, glm::mat4(1.0f)});
    while (!nodeIndices.empty()) {
        auto index = nodeIndices.back().index;
        auto parentTForm = nodeIndices.back().tform;
        nodeIndices.pop_back();

        glm::mat4 tform = mNodes[index].transform;
        int32_t boneIndex = mNodes[index].boneIndex;
        int32_t channelIndex = mNodes[index].channelIndex;
        if (channelIndex != -1) {
            auto & channel = animation.channels[channelIndex];

            glm::vec3 const & prevPos = channel.keys[prevFrame].position;
            glm::vec3 const & nextPos = channel.keys[nextFrame].position;

            glm::quat const & prevRot = channel.keys[prevFrame].rotation;
            glm::quat const & nextRot = channel.keys[nextFrame].rotation;

            glm::vec3 const & prevScale = channel.keys[prevFrame].scaling;
            glm::vec3 const & nextScale = channel.keys[nextFrame].scaling;

            glm::vec3 pos = glm::lerp(prevPos, nextPos, frac);
            glm::quat rot = glm::slerp(prevRot, nextRot, frac);
            glm::vec3 scale = glm::lerp(prevScale, nextScale, frac);
            tform = glm::translate(pos) * glm::toMat4(rot) * glm::scale(scale);
        }
        // pose matrix
        glm::mat4 poseMatrix = parentTForm * tform;

        if (boneIndex != -1) {
            transforms[boneIndex] = poseMatrix * bones[boneIndex].offsetMatrix;
        }

        for (auto & childIndex : mNodes[index].childIndices) {
            nodeIndices.push_back({childIndex, poseMatrix});
        }
    }
}

void Model::getTexture(int32_t &textureIndex, aiMaterial &aiMat, aiTextureType type,
                       prt::hash_map<aiString, size_t> &map, const char * modelPath) {
    aiString texPath;
    if (aiMat.GetTexture(type, 0, &texPath) == AI_SUCCESS) {
        if (type != aiTextureType_DIFFUSE) std::cout << "NORMALS" << std::endl;
        char fullTexPath[256];
        strcpy(fullTexPath, modelPath);
        auto it = map.find(texPath);
        if (it != map.end()) {
            textureIndex = it->value();
        } else {
            size_t index = textures.size();
            map.insert(texPath, index);
            textures.push_back({});
            Texture &tex = textures.back();
            
            char *ptr = strrchr(fullTexPath, '/');
            strcpy(++ptr, texPath.C_Str());
            tex.load(fullTexPath);

            textureIndex = index;
        }
    }
}

void Model::calcTangentSpace() {
    for (size_t i = 0; i < indexBuffer.size(); i+=3) {
        auto & v0 = vertexBuffer[indexBuffer[i]];
        auto & v1 = vertexBuffer[indexBuffer[i+1]];
        auto & v2 = vertexBuffer[indexBuffer[i+2]];

        glm::vec3 edge1 = v1.pos - v0.pos;
        glm::vec3 edge2 = v2.pos - v0.pos;
        glm::vec2 deltaUV1 = v1.texCoord - v0.texCoord;
        glm::vec2 deltaUV2 = v2.texCoord - v0.texCoord;  
        
        glm::vec3 tan;
        glm::vec3 bi;

        float denom = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        if (denom == 0) {
            continue;
        }
        float f = 1.0f / denom;


        tan.x = f * (deltaUV2.y * edge1.x - deltaUV1.y * edge2.x);
        tan.y = f * (deltaUV2.y * edge1.y - deltaUV1.y * edge2.y);
        tan.z = f * (deltaUV2.y * edge1.z - deltaUV1.y * edge2.z);
        // tan = glm::normalize(tan);
        v0.tangent += tan; 
        v1.tangent += tan;
        v2.tangent += tan;

        bi.x = f * ((-deltaUV2.x * edge1.x) + deltaUV1.x * edge2.x);
        bi.y = f * ((-deltaUV2.x * edge1.y) + deltaUV1.x * edge2.y);
        bi.z = f * ((-deltaUV2.x * edge1.z) + deltaUV1.x * edge2.z);
        // bi = glm::normalize(bi); 
        v0.bitangent += bi; 
        v1.bitangent += bi; 
        v2.bitangent += bi; 
    }
    for (auto & vert : vertexBuffer) {
        if (glm::length(vert.tangent) == 0.0f) continue;
        vert.tangent = normalize(vert.tangent);
        vert.tangent = glm::normalize(vert.tangent - (vert.normal * glm::dot(vert.normal, vert.tangent)));
        glm::vec3 c = glm::cross(vert.normal, vert.tangent);
        if (glm::dot(c, vert.bitangent) < 0) {
            vert.tangent = -vert.tangent;
        }
        vert.bitangent = glm::cross(vert.normal, vert.tangent);
    }
}

VkVertexInputBindingDescription Model::Vertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(Vertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return bindingDescription;
}

prt::vector<VkVertexInputAttributeDescription> Model::Vertex::getAttributeDescriptions() {
    prt::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
    attributeDescriptions.resize(5);
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 12;
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 24;

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = 32;

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = 44;
    
    return attributeDescriptions;
}

VkVertexInputBindingDescription Model::BonedVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(BonedVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return bindingDescription;
}

prt::vector<VkVertexInputAttributeDescription> Model::BonedVertex::getAttributeDescriptions() {
    prt::vector<VkVertexInputAttributeDescription> attributeDescriptions = {};
    attributeDescriptions.resize(7);
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = 0;
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = 12;
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = 24;

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = 32;

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = 44;

    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_UINT;
    attributeDescriptions[5].offset = 56;

    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = 72;
    
    return attributeDescriptions;
}
