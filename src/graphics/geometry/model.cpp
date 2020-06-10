#include "model.h"

#include "src/graphics/geometry/fbx_scene.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <glm/gtx/transform.hpp> 
#include <glm/gtx/euler_angles.hpp>
#include <glm/gtx/string_cast.hpp>

// #include <assimp/cimport.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include <fstream>

void Texture::load(char const * path) {
    stbi_uc* pixels = stbi_load(path, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        std::cout << "path: " << path << std::endl;
        assert(false && "failed to load texture image!");
    }

    size_t bufferSize = texWidth * texHeight * 4;
    pixelBuffer.resize(bufferSize);
    for (size_t i = 0; i < pixelBuffer.size(); i++) {
        pixelBuffer[i] = pixels[i];
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    stbi_image_free(pixels);
}

void Model::load(char const * path) {
    assert(!mLoaded && "Model is already loaded!");
    
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
    prt::hash_map<aiString, size_t> texturePathToIndex;
    materials.resize(scene->mNumMaterials);
    for (size_t i = 0; i < materials.size(); ++i) {
        aiString matName;
        aiGetMaterialString(scene->mMaterials[i], AI_MATKEY_NAME, &matName);
        strcpy(materials[i].name, matName.C_Str());
 
        aiString texPath;
        if (scene->mMaterials[i]->GetTexture(aiTextureType_DIFFUSE, 0, &texPath) == AI_SUCCESS) {
            char fullTexPath[256];
            strcpy(fullTexPath, path);
            aiColor3D color;
            scene->mMaterials[i]->Get(AI_MATKEY_COLOR_DIFFUSE, color);
            materials[i].baseColor = { color.r, color.g, color.b };
            auto it = texturePathToIndex.find(texPath);
            if (it != texturePathToIndex.end()) {
                materials[i].albedoIndex = it->value();
            } else {
                size_t index = textures.size();
                texturePathToIndex.insert(texPath, index);
                textures.push_back({});
                Texture &tex = textures.back();
                
                char *ptr = strrchr(fullTexPath, '/');
                strcpy(++ptr, texPath.C_Str());
                tex.load(fullTexPath);

                materials[i].albedoIndex = index;
            }
        }
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

                // aiVector3D tan = (invtpos * aiMesh->mTangents[j]).Normalize();
                // vertexBuffer[vert].tangent.x = tan.x;
                // vertexBuffer[vert].tangent.y = tan.y;
                // vertexBuffer[vert].tangent.z = tan.z;

                // aiVector3D bitan = (invtpos * aiMesh->mBitangents[j]).Normalize();
                // vertexBuffer[vert].bitangent.x = bitan.x;
                // vertexBuffer[vert].bitangent.y = bitan.y;
                // vertexBuffer[vert].bitangent.z = bitan.z;
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
        }
        // then do the same for each of its children
        for(size_t i = 0; i < node->mNumChildren; ++i) {
            nodes.push_back({node->mChildren[i], tform * node->mChildren[i]->mTransformation});
        }
    }

    // release assimp resources
    mLoaded = true;
    calcTangentSpace();
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
        
        float denom = (deltaUV1.x * deltaUV2.y - deltaUV2.x * deltaUV1.y);
        float f = denom != 0.0f ? 1.0f / denom : 0.0f;

        glm::vec3 tan;
        glm::vec3 bi;

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
        vert.tangent = vert.tangent.length() != 0.0f ? normalize(vert.tangent) : glm::vec3{ 0.0f, 1.0f, 0.0f };
        vert.tangent = glm::normalize(vert.tangent - (vert.normal * glm::dot(vert.normal, vert.tangent)));
        glm::vec3 c = glm::cross(vert.normal, vert.tangent);
        if (glm::dot(c, vert.bitangent) < 0) {
            vert.tangent = -vert.tangent;
        }
        vert.bitangent = glm::cross(vert.normal, vert.tangent);
    }
}
