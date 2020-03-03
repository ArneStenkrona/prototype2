#include "model.h"

#include "src/container/hash_map.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb/stb_image.h>

#include <fstream>

void Texture::load(const char* texturePath) {
    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    size_t bufferSize = texWidth * texHeight * 4;
    pixelBuffer.resize(bufferSize);
    for (size_t i = 0; i < pixelBuffer.size(); i++) {
        pixelBuffer[i] = pixels[i];
    }

    mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(texWidth, texHeight)))) + 1;
    stbi_image_free(pixels);
}

// Helper function for counting meshes and indices in obj file.
void countAttributes(FILE* file, size_t& numMesh, size_t& numIndex) {
    char lineHeader[512];
    int res = fscanf(file, "%s%*[^\n]", lineHeader);
    // Count objects, vertices and indices.
    while (res != EOF) {
        // read a line and save only the first word
        if (strcmp(lineHeader, "o") == 0) {
            numMesh++;
        } else if (strcmp(lineHeader, "v") == 0) {
            //numVertex++;
        } else if (strcmp(lineHeader, "f") == 0) {
            // We assume that each face is a triangle
            numIndex += 3;
        }
        // Get next header
        res = fscanf(file, "%s%*[^\n]", lineHeader);
    }
    numMesh = numMesh == 0 ? 1 : numMesh;
    rewind(file);
}

void Model::loadOBJ(const char* path) {
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", path);
        printf("'!\n");
        assert(false);
        return;
    }

    size_t numMesh = 0;
    size_t numIndex = 0;

    countAttributes(file, numMesh, numIndex);
    
    meshes.resize(numMesh);
    indexBuffer.resize(numIndex);

    // Currently supports 2^16 vertices
    constexpr size_t VERTEX_BUFFER_SIZE = UINT16_MAX;

    // Temporary vertex buffer
    Vertex vertexBufferTemp[VERTEX_BUFFER_SIZE];
    prt::hash_map<Vertex, uint32_t> uniqueVertices; 

    // Variables for counting
    size_t meshCount = 0;
    // These are for the temps
    size_t vertexPosCount = 0;
    size_t vertexNormalCount = 0;
    size_t vertexTexCoordCount = 0;
    // these are for when inserting into the models buffers
    size_t vertexCount = 0;
    size_t indexCount = 0;

    // An obj-file with a single object may not have
    // an "o" header.
    if (numMesh == 1) {
        meshes[0].numIndices = numIndex;
        meshes[0].startIndex = 0;
    }
    // Parse the contents.
    char lineHeader[512];
    char mtllib[512];
    uint32_t assignedMaterials = 0;
    int res = fscanf(file, "%s", lineHeader);
    size_t lineNumber = 0;
    while (res != EOF) {
        lineNumber++;
        // read the first word of the line
        if (strcmp(lineHeader, "o") == 0) {
            // object name
            fscanf(file, "%s", meshes[meshCount].name);

            // Object.
            fscanf(file, "%*[^\n]\n", NULL);

            meshes[meshCount].startIndex = indexCount; 
            meshes[meshCount].numIndices = numIndex - indexCount;
            if (meshCount > 0) {
                meshes[meshCount - 1].numIndices = indexCount - meshes[meshCount - 1].startIndex;   
            }
            meshCount++;
        } else if (strcmp(lineHeader, "mtllib") == 0) {
            // Parse material file.
            fscanf(file, "%s", mtllib);
        } else if (strcmp(lineHeader, "usemtl") == 0) {
            // Parse mesh material.
            char currentMtl[256];
            fscanf(file, "%s", currentMtl);
            while (assignedMaterials < meshCount) {
                strcpy(meshes[assignedMaterials++].material.name, currentMtl);
            }
        } else if (strcmp(lineHeader, "v") == 0) {
            // Parse vertex position.
            glm::vec3& pos = vertexBufferTemp[vertexPosCount++].pos;
            fscanf(file, "%f %f %f\n", &pos.x, &pos.y, &pos.z );
        } else if (strcmp(lineHeader, "vn") == 0) {
            // Parse vertex normal.
            glm::vec3& normal = vertexBufferTemp[vertexNormalCount++].normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
        } else if (strcmp(lineHeader, "vt") == 0) {
            // Parse vertex texture coordinate.
            glm::vec2& texCoord = vertexBufferTemp[vertexTexCoordCount++].texCoord;
            fscanf(file, "%f %f\n", &texCoord.x, &texCoord.y );
            // OBJ and Vulkan have relative flipped y axis for textures
            texCoord.y = 1.0f - texCoord.y;
        } else if (strcmp(lineHeader, "f") == 0) {
            // Parse face.
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
                                 &vertexIndex[0], &uvIndex[0], &normalIndex[0], 
                                 &vertexIndex[1], &uvIndex[1], &normalIndex[1], 
                                 &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            if (matches != 9) {
                printf("File can't be read by our simple parser : ( Try exporting with other options\n");
                std::cout << lineNumber << std::endl;
                assert(false);
                return;
            }

            Vertex vertices[3] = {
                { vertexBufferTemp[vertexIndex[0] - 1].pos,
                  vertexBufferTemp[normalIndex[0] - 1].normal,
                  vertexBufferTemp[uvIndex[0] - 1].texCoord },

                { vertexBufferTemp[vertexIndex[1] - 1].pos,
                  vertexBufferTemp[normalIndex[1] - 1].normal,
                  vertexBufferTemp[uvIndex[1] - 1].texCoord },

                { vertexBufferTemp[vertexIndex[2] - 1].pos,
                  vertexBufferTemp[normalIndex[2] - 1].normal,
                  vertexBufferTemp[uvIndex[2] - 1].texCoord }
            };

            for (unsigned int i = 0; i < 3; i++) {
                if (uniqueVertices.find(vertices[i]) == uniqueVertices.end()) {
                    uniqueVertices[vertices[i]] = static_cast<uint32_t>(vertexCount++);
                }
                indexBuffer[indexCount++] = uniqueVertices[vertices[i]];
            }        
        }
     
        res = fscanf(file, "%s", lineHeader);
    }
    fclose(file);

    size_t numVertex = uniqueVertices.size();
    vertexBuffer.resize(numVertex);
    
    indexCount = 0;
    for (auto it = uniqueVertices.begin(); it != uniqueVertices.end(); it++) {
        vertexBuffer[it->value()] = it->key();
    }

    // load materials
    if (mtllib[0] == '\0') {
        return;
    }
    char mtlPath[512];
    strcpy(mtlPath, path);
    char *ptr = strrchr(mtlPath, '/');
    if (!ptr) {
        assert(false);
    }
    strcpy(++ptr, mtllib);


    FILE* materialFile = fopen(mtlPath, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", path);
        printf("'!\n");
        assert(false);
        return;
    }

    prt::hash_map<std::string, std::string> mtlToTexture;
    prt::hash_map<std::string, std::string> mtlToFragShader;

    res = fscanf(materialFile, "%s", lineHeader);
    char material[512];
    while (res != EOF) {
        if (strcmp(lineHeader, "newmtl") == 0) {
            // Parse material name.
            fscanf(file, "%s", material);
        } else if (strcmp(lineHeader, "map_Kd") == 0) {
            // Parse texture path.
            char texture[256];
            fscanf(file, "%s", texture);
            mtlToTexture.insert(material, texture);
        } else if (strcmp(lineHeader, "fragment_shader") == 0) {
            // Parse mesh material.
            char fragShader[256];
            fscanf(file, "%s", fragShader);
            mtlToFragShader.insert(material, fragShader);
        } 

        res = fscanf(materialFile, "%s", lineHeader);
    }
    fclose(materialFile);

    for (auto & mesh : meshes) {
        if (mtlToFragShader.find(mesh.material.name) == mtlToFragShader.end()) {
            strcpy(mesh.material.fragmentShader, "cel.frag");
        } else {
            strcpy(mesh.material.fragmentShader, mtlToFragShader[mesh.material.name].c_str());
        }

        char texturePath[256];
        strcpy(texturePath, path);
        ptr = strrchr(texturePath, '/');
        strcpy(++ptr, mtlToTexture[mesh.material.name].c_str());
        mesh.texture.load(texturePath);
    }
}   