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

Model::Model(char const * path)
    : mLoaded(false), mAnimated(false) {
    strcpy(mPath, path);
}

bool Model::load(bool loadAnimation, TextureManager & textureManager) {
    assert(!mLoaded && "Model is already loaded!");

    mAnimated = loadAnimation;

    Assimp::Importer importer;
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, 
                                aiPrimitiveType_LINE | aiPrimitiveType_POINT);
    
    aiScene const * scene = importer.ReadFile(mPath,
                                              aiProcess_CalcTangentSpace         |
                                              aiProcess_Triangulate              |
                                              aiProcess_FlipUVs                  |
                                              aiProcess_FindDegenerates          |
                                              aiProcess_JoinIdenticalVertices    |
                                              aiProcess_RemoveRedundantMaterials |
                                              aiProcess_ImproveCacheLocality     |
                                              aiProcess_SortByPType);

    // check if import failed
    if(!scene) {
        std::cout << importer.GetErrorString() << std::endl;
        // assert(false && "failed to load file!");
        return false;
    }
    
    strcpy(name, strrchr(mPath, '/') + 1);

    // parse materials
    materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < materials.size(); ++i) {
        aiString matName;
        aiGetMaterialString(scene->mMaterials[i], AI_MATKEY_NAME, &matName);
        strcpy(materials[i].name, matName.C_Str());
 
        aiColor3D color;
        scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);

        materials[i].baseColor = { color.r, color.g, color.b, 1.0f };
        
        scene->mMaterials[i]->Get(AI_MATKEY_OPACITY, materials[i].baseColor.a);
        scene->mMaterials[i]->Get(AI_MATKEY_TWOSIDED, materials[i].twosided);

        
        if (materials[i].baseColor.a < 1.0f) {
            materials[i].type = Material::Type::transparent;
        }
        if (strstr(materials[i].name, "[water]") != NULL) {
            materials[i].type = Material::Type::water;
        }
        if (strstr(materials[i].name, "[transparent]") != NULL) {
            materials[i].type = Material::Type::transparent;
        }
        if (strstr(materials[i].name, "[gloss]") != NULL) {
            materials[i].baseSpecularity = 1.0f;
        }

        materials[i].albedoIndex = getTexture(*scene->mMaterials[i], aiTextureType_DIFFUSE, mPath, textureManager);
        materials[i].normalIndex = getTexture(*scene->mMaterials[i], aiTextureType_NORMALS, mPath, textureManager);
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
    
    prt::vector<aiString> boneToName;

    prt::hash_map<aiString, int> boneMapping;

    prt::vector<TFormNode> nodes;
    nodes.push_back({scene->mRootNode, scene->mRootNode->mTransformation, -1});
    while(!nodes.empty()) {
        aiNode *node = nodes.back().node;
        int32_t parentIndex = nodes.back().parentIndex;
        aiMatrix4x4 tform = nodes.back().tform;
        aiMatrix3x3 invtpos = aiMatrix3x3(tform);
        invtpos.Inverse().Transpose();
        nodes.pop_back();

        // add node member
        int32_t nodeIndex = mNodes.size();
        mNodes.push_back({});
        Node & n = mNodes.back();
        n.name = node->mName;
        memcpy(&n.transform, &node->mTransformation, sizeof(glm::mat4));
        // assimp row-major, glm col-major
        n.transform = glm::transpose(n.transform);
        nameToNode.insert(node->mName, nodeIndex);
        
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
                boneToName.resize(prevBoneSize + aiMesh->mNumBones);

                for (size_t j = 0; j < aiMesh->mNumBones; ++j) {
                    size_t bi = prevBoneSize + j;
                    aiBone const * bone = aiMesh->mBones[j];

                    boneToName[bi] = bone->mName;
                    nameToBone[bone->mName] = bi;

                    memcpy(&bones[bi].offsetMatrix, &bone->mOffsetMatrix, sizeof(glm::mat4));

                    memcpy(&bones[bi].meshTransform, &node->mTransformation, sizeof(glm::mat4));
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
            
            // trim names such as "armature|<animationName>"
            const char * toCopy = strchr(aiAnim->mName.C_Str(), '|');
            if (toCopy != nullptr) {
                char nameBuf[256];
                ++toCopy;
                strcpy(nameBuf, toCopy);
                nameToAnimation.insert(aiString(nameBuf), i);
            } else {
                nameToAnimation.insert(aiAnim->mName, i);
            }

            Animation & anim = animations[i];
            anim.duration = aiAnim->mDuration;
            anim.ticksPerSecond = aiAnim->mTicksPerSecond;
            anim.channels.resize(aiAnim->mNumChannels);

            for (size_t j = 0; j < aiAnim->mNumChannels; ++j) {
                aiNodeAnim const * aiChannel = aiAnim->mChannels[j];
                AnimationNode & channel = anim.channels[j];

                assert(nameToNode.find(aiChannel->mNodeName) != nameToNode.end() && "animation does not correspond to node");
                auto nodeIndex = nameToNode.find(aiChannel->mNodeName)->value();
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
            aiString & boneName = boneToName[i];
            
            assert(nameToNode.find(boneName) != nameToNode.end() && "No corresponding node for bone");
            size_t nodeIndex = nameToNode.find(boneName)->value();
            mNodes[nodeIndex].boneIndices.push_back(i);
        }
    }

    calcTangentSpace();

    mLoaded = true;
    return true;
}

int Model::getAnimationIndex(char const * name) const {
    if (nameToAnimation.find(aiString(name)) == nameToAnimation.end()) {
        return -1;
    }
    return nameToAnimation.find(aiString(name))->value();
}

int Model::getBoneIndex(char const * name) const {
    if (nameToBone.find(aiString(name)) == nameToBone.end()) {
        return -1;
    }
    return nameToBone.find(aiString(name))->value();
}

glm::mat4 Model::getBoneTransform(int index) const {
    return glm::inverse(bones[index].offsetMatrix);
}


glm::mat4 Model::getBoneTransform(char const * name) const {
    if (nameToNode.find(aiString(name)) == nameToNode.end()) {
        assert(false && "No bone by that name!");
    }

    int index = nameToBone.find(aiString(name))->value();
    return getBoneTransform(index);
}

void Model::sampleAnimation(float t, size_t animationIndex, glm::mat4 * transforms) const {
    assert(mAnimated);
    auto const & animation = animations[animationIndex];

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
        int32_t channelIndex = mNodes[index].channelIndex;

        if (channelIndex != -1) {
            auto & channel = animation.channels[channelIndex];

            // calculate prev and next frame
            float duration = animation.duration / (1000 * animation.ticksPerSecond);
            float animTime = t / duration;
            size_t numFrames = animation.channels[channelIndex].keys.size();
            float fracFrame = animTime * numFrames;
            uint32_t prevFrame = static_cast<uint32_t>(fracFrame);
            float frac = fracFrame - prevFrame;
            prevFrame = prevFrame % numFrames;
            uint32_t nextFrame = (prevFrame + 1) % numFrames;

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

        for (auto const & boneIndex : mNodes[index].boneIndices) {
            transforms[boneIndex] = poseMatrix * bones[boneIndex].offsetMatrix;
        }

        for (auto const & childIndex : mNodes[index].childIndices) {
            nodeIndices.push_back({childIndex, poseMatrix});
        }
    }
}

void Model::blendAnimation(float t, 
                           float blendFactor,
                           size_t animationIndexA, 
                           size_t animationIndexB,
                           glm::mat4 * transforms) const {
    assert(mAnimated);
    auto const & animationA = animations[animationIndexA];
    auto const & animationB = animations[animationIndexB];

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
        int32_t channelIndex = mNodes[index].channelIndex;

        if (channelIndex != -1) {
            auto & channelA = animationA.channels[channelIndex];
            auto & channelB = animationB.channels[channelIndex];

            float durationA = animationA.duration / animationA.ticksPerSecond;
            float clipTimeA = t / durationA;

            float durationB = animationB.duration / animationB.ticksPerSecond;
            float clipTimeB = t / durationB;

            // calculate prev and next frame for clip A
            size_t numFramesA = animationA.channels[channelIndex].keys.size();
            float fracFrameA = clipTimeA * numFramesA;
            uint32_t prevFrameA = static_cast<uint32_t>(fracFrameA);
            float fracA = fracFrameA - prevFrameA;
            prevFrameA = prevFrameA % numFramesA;
            uint32_t nextFrameA = (prevFrameA + 1) % numFramesA;

            // calculate prev and next frame for clip B
            size_t numFramesB = animationB.channels[channelIndex].keys.size();
            float fracFrameB = clipTimeB * numFramesB;
            uint32_t prevFrameB = static_cast<uint32_t>(fracFrameB);
            float fracB = fracFrameB - prevFrameB;
            prevFrameB = prevFrameB % numFramesB;
            uint32_t nextFrameB = (prevFrameB + 1) % numFramesB;
            
            // clip A
            glm::vec3 const & prevPosA = channelA.keys[prevFrameA].position;
            glm::vec3 const & nextPosA = channelA.keys[nextFrameA].position;

            glm::quat const & prevRotA = channelA.keys[prevFrameA].rotation;
            glm::quat const & nextRotA = channelA.keys[nextFrameA].rotation;

            glm::vec3 const & prevScaleA = channelA.keys[prevFrameA].scaling;
            glm::vec3 const & nextScaleA = channelA.keys[nextFrameA].scaling;

            glm::vec3 posA = glm::lerp(prevPosA, nextPosA, fracA);
            glm::quat rotA = glm::slerp(prevRotA, nextRotA, fracA);
            glm::vec3 scaleA = glm::lerp(prevScaleA, nextScaleA, fracA);

            // clip B
            glm::vec3 const & prevPosB = channelB.keys[prevFrameB].position;
            glm::vec3 const & nextPosB = channelB.keys[nextFrameB].position;

            glm::quat const & prevRotB = channelB.keys[prevFrameB].rotation;
            glm::quat const & nextRotB = channelB.keys[nextFrameB].rotation;

            glm::vec3 const & prevScaleB = channelB.keys[prevFrameB].scaling;
            glm::vec3 const & nextScaleB = channelB.keys[nextFrameB].scaling;

            glm::vec3 posB = glm::lerp(prevPosB, nextPosB, fracB);
            glm::quat rotB = glm::slerp(prevRotB, nextRotB, fracB);
            glm::vec3 scaleB = glm::lerp(prevScaleB, nextScaleB, fracB);

            // blend
            glm::vec3 pos = glm::lerp(posA, posB, blendFactor);
            glm::quat rot = glm::slerp(rotA, rotB, blendFactor);
            glm::vec3 scale = glm::lerp(scaleA, scaleB, blendFactor);

            tform = glm::translate(pos) * glm::toMat4(rot) * glm::scale(scale);
        }
        // pose matrix
        glm::mat4 poseMatrix = parentTForm * tform;

        for (auto const & boneIndex : mNodes[index].boneIndices) {
            transforms[boneIndex] = poseMatrix * bones[boneIndex].offsetMatrix;
        }

        for (auto const & childIndex : mNodes[index].childIndices) {
            nodeIndices.push_back({childIndex, poseMatrix});
        }
    }                            
}

int32_t Model::getTexture(aiMaterial &aiMat, aiTextureType type, const char * modelPath,
                          TextureManager & textureManager) {
    aiString texPath;
    int32_t id = -1;
    if (aiMat.GetTexture(type, 0, &texPath) == AI_SUCCESS) {
        char fullTexPath[256];
        strcpy(fullTexPath, modelPath);

        char *ptr = strrchr(fullTexPath, '/');
        strcpy(++ptr, texPath.C_Str());
        id = textureManager.loadTexture(fullTexPath, true);
    }

    return id;
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
