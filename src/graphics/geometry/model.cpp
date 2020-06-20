#include "model.h"

#include "src/graphics/geometry/fbx_scene.h"

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

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
    
    const aiScene* scene = importer.ReadFile(path,
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

    struct TFormNode {
        aiNode* node;
        aiMatrix4x4 tform;
    };
    prt::vector<TFormNode> nodes;
    nodes.push_back({scene->mRootNode, scene->mRootNode->mTransformation});
    while(!nodes.empty()) {
        aiNode *node = nodes.back().node;
        aiMatrix4x4 &tform = nodes.back().tform;
        aiMatrix3x3 invtpos = aiMatrix3x3(tform);
        invtpos.Inverse().Transpose();
        nodes.pop_back();
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
                mesh.bones.resize(aiMesh->mNumBones);
                for (size_t j = 0; j < aiMesh->mNumBones; ++j) {
                    aiBone const * bone = aiMesh->mBones[i];
                    memcpy(&mesh.bones[j].offsetMatrix, &bone->mOffsetMatrix, sizeof(mesh.bones[j]));
                    // store the bone weights and IDs in vertices
                    for (size_t iv = 0; iv < bone->mNumWeights; ++iv) {
                        BoneData & bd = vertexBoneBuffer[prevVertSize + bone->mWeights[iv].mVertexId];
                        // only store the 4 most influential bones
                        int leastInd = -1;
                        float weight = bone->mWeights[iv].mWeight;
                        float leastWeight = weight;
                        for (size_t ind = 0; ind < 4; ++ind) {
                            if (bd.boneIDs[ind] == -1) {
                                leastInd = ind;
                                break;
                            } else if (bd.boneWeights[ind] < leastWeight) {
                                leastWeight = bd.boneWeights[ind];
                                leastInd = ind;
                            }
                        }
                        if (leastInd != -1) {
                            bd.boneIDs[leastInd] = j;
                            bd.boneWeights[leastInd] = weight;
                        }
                    }
                }
            }
        }
        // process all children of the node
        for (size_t i = 0; i < node->mNumChildren; ++i) {
            nodes.push_back({node->mChildren[i], tform * node->mChildren[i]->mTransformation});
        }
    }
    // parse animations
    if (loadAnimation) {
        animations.resize(scene->mNumAnimations);
        for (size_t i = 0; i < scene->mNumAnimations; ++i) {
            aiAnimation const * aiAnim = scene->mAnimations[i];
            Animation & anim = animations[i];
            anim.duration = aiAnim->mDuration;
            anim.ticksPerSecond = aiAnim->mTicksPerSecond;
            anim.channels.resize(aiAnim->mNumChannels);
            for (size_t j = 0; j < aiAnim->mNumChannels; ++j) {
                aiNodeAnim const * aiChannel = aiAnim->mChannels[j];
                AnimationNode & channel = anim.channels[j];

                assert(aiChannel->mNumPositionKeys == aiChannel->mNumRotationKeys && 
                    aiChannel->mNumPositionKeys == aiChannel->mNumScalingKeys && "number of position, rotation and scaling keys need to match");
                channel.keys.resize(aiChannel->mNumPositionKeys);

                for (size_t k = 0; k < channel.keys.size(); ++k) {
                    aiVector3D const & aiPos = aiChannel->mPositionKeys[k].mValue;
                    aiQuaternion const & aiRot = aiChannel->mRotationKeys[k].mValue;
                    //aiVector3D const & aiScale = aiChannel->mScalingKeys[k].mValue;
                    channel.keys[k].position = { aiPos.x, aiPos.y, aiPos.z };
                    channel.keys[k].rotation = { aiRot.x, aiRot.y, aiRot.z, aiRot.w };
                    //channel.keys[k].scaling = { aiScale.x, aiScale.y, aiScale.z };
                }
            }
        }
    }

    // release assimp resources
    mLoaded = true;
    calcTangentSpace();
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

prt::array<VkVertexInputAttributeDescription, 5> Model::Vertex::getAttributeDescriptions() {
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

VkVertexInputBindingDescription Model::BonedVertex::getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription = {};
    bindingDescription.binding = 0;
    bindingDescription.stride = sizeof(BonedVertex);
    bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;
    
    return bindingDescription;
}

prt::array<VkVertexInputAttributeDescription, 7> Model::BonedVertex::getAttributeDescriptions() {
    prt::array<VkVertexInputAttributeDescription, 7> attributeDescriptions = {};
    
    attributeDescriptions[0].binding = 0;
    attributeDescriptions[0].location = 0;
    attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[0].offset = offsetof(BonedVertex, pos);
    
    attributeDescriptions[1].binding = 0;
    attributeDescriptions[1].location = 1;
    attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[1].offset = offsetof(BonedVertex, normal);
    
    attributeDescriptions[2].binding = 0;
    attributeDescriptions[2].location = 2;
    attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
    attributeDescriptions[2].offset = offsetof(BonedVertex, texCoord);

    attributeDescriptions[3].binding = 0;
    attributeDescriptions[3].location = 3;
    attributeDescriptions[3].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[3].offset = offsetof(BonedVertex, tangent);

    attributeDescriptions[4].binding = 0;
    attributeDescriptions[4].location = 4;
    attributeDescriptions[4].format = VK_FORMAT_R32G32B32_SFLOAT;
    attributeDescriptions[4].offset = offsetof(BonedVertex, bitangent);

    attributeDescriptions[5].binding = 0;
    attributeDescriptions[5].location = 5;
    attributeDescriptions[5].format = VK_FORMAT_R32G32B32A32_SINT;
    attributeDescriptions[5].offset = offsetof(BonedVertex, boneData.boneIDs);

    attributeDescriptions[6].binding = 0;
    attributeDescriptions[6].location = 6;
    attributeDescriptions[6].format = VK_FORMAT_R32G32B32A32_SFLOAT;
    attributeDescriptions[6].offset = offsetof(BonedVertex, boneData.boneWeights);
    
    return attributeDescriptions;
}
