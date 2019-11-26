#include "model.h"

#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>

#include "src/container/hash_map.h"

#include <iostream>

// Helper function for counting _meshes and indices in obj file.
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

Model::Model(std::string& path)
: _path(path),
  _vertexBuffer(4 * sizeof(float)),
  _indexBuffer(sizeof(uint32_t)),
  _meshes(sizeof(size_t)),
  _meshesAreLoaded(false),
  _texturesAreLoaded(false) {}

void Model::load() {
    loadMeshes();
    loadTextures();
}

void Model::loadMeshes() {

    std::string modelPath = _path + "model.obj";

    FILE* file = fopen((modelPath).c_str(), "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", modelPath.c_str());
        printf("'!\n");
        assert(false);
        return;
    }

    size_t numMesh = 0;
    size_t numIndex = 0;

    countAttributes(file, numMesh, numIndex);
    
    _meshes.resize(numMesh);
    _indexBuffer.resize(numIndex);

    // Currently supports 2^16 vertices
    constexpr size_t VERTEX_BUFFER_SIZE = UINT16_MAX;

    // Temporary vertex buffer
    Vertex vertexBufferTemp[VERTEX_BUFFER_SIZE];
    //std::unordered_map<Vertex, uint32_t> uniqueVertices = {}; 
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
    if (_meshes.size() == 1) {
        _meshes[0].numIndices = _indexBuffer.size();
        _meshes[0].startIndex = 0;
    }

    // Parse the contents.
    char lineHeader[512];
    int res = fscanf(file, "%s", lineHeader);
    while (res != EOF) {
        // read the first word of the line
        if (res == EOF) {
            break; // break when we've reached end of file
        }

        if (strcmp(lineHeader, "o") == 0) {
            // Object.
            fscanf(file, "%*[^\n]\n", NULL);

            if (meshCount > 0) {
                _meshes[meshCount].startIndex = 
                    _meshes[meshCount - 1].startIndex + _meshes[meshCount - 1].numIndices;
                _meshes[meshCount - 1].numIndices = 
                    indexCount - _meshes[meshCount - 1].startIndex;                
            } else {
                _meshes[meshCount].startIndex = 0;
            }
            // This will be overwritten unless we've reached
            // the final mesh.
            _meshes[meshCount].numIndices = 
                indexCount - _indexBuffer.size();

            meshCount++;

        } else if (strcmp(lineHeader, "v") == 0) {
            // Parse vertex position.
            glm::vec3& pos = vertexBufferTemp[vertexPosCount].pos;
            fscanf(file, "%f %f %f\n", &pos.x, &pos.y, &pos.z );
            vertexPosCount++;
        } else if (strcmp(lineHeader, "vn") == 0) {
            // Parse vertex normal.
            glm::vec3& normal = vertexBufferTemp[vertexNormalCount].normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            vertexNormalCount++;
        } else if (strcmp(lineHeader, "vt") == 0) {
            // Parse vertex texture coordinate.
            glm::vec2& texCoord = vertexBufferTemp[vertexTexCoordCount].texCoord;
            fscanf(file, "%f %f\n", &texCoord.x, &texCoord.y );
            // OBJ and Vulkan have relative flipped y axis for textures
            texCoord.y = 1.0f - texCoord.y;
            vertexTexCoordCount++;
        } else if (strcmp(lineHeader, "f") == 0) {
            // Parse face.
            unsigned int vertexIndex[3], uvIndex[3], normalIndex[3];
            int matches = fscanf(file, "%d/%d/%d %d/%d/%d %d/%d/%d\n", 
                                 &vertexIndex[0], &uvIndex[0], &normalIndex[0], 
                                 &vertexIndex[1], &uvIndex[1], &normalIndex[1], 
                                 &vertexIndex[2], &uvIndex[2], &normalIndex[2] );
            if (matches != 9) {
                printf("File can't be read by our simple parser : ( Try exporting with other options\n");
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
                _indexBuffer[indexCount++] = uniqueVertices[vertices[i]];
            }        
        }
     
        res = fscanf(file, "%s", lineHeader);
    }
    size_t numVertex = uniqueVertices.size();
    _vertexBuffer.resize(numVertex);
    
    indexCount = 0;
    for (auto it = uniqueVertices.begin(); it != uniqueVertices.end(); it++) {
        _vertexBuffer[it->value()] = it->key();
    }
    _meshesAreLoaded = true;
}

void Model::loadTextures() {
    int texWidth, texHeight, texChannels;
    stbi_uc* pixels = stbi_load((_path + "diffuse.png").c_str(), &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    _texture.texWidth = texWidth;
    _texture.texHeight = texHeight;
    _texture.texChannels = texChannels;

    size_t bufferSize = texWidth * texHeight * 4;
    _texture.pixelBuffer.resize(bufferSize);
    for (size_t i = 0; i < _texture.pixelBuffer.size(); i++) {
        _texture.pixelBuffer[i] = pixels[i];
    }

    stbi_image_free(pixels);
    _texturesAreLoaded = true;
}

void Model::free() {
    freeMeshes();
    freeTextures();
}

void Model::freeMeshes() {
    _vertexBuffer = prt::vector<Vertex>();
    _indexBuffer = prt::vector<uint32_t>();
    _meshes = prt::vector<Mesh>();

    _meshesAreLoaded = false;
}

void Model::freeTextures() {
    _texture.pixelBuffer = prt::vector<unsigned char>();
    _texture.texWidth = 0;
    _texture.texHeight = 0;
    _texture.texChannels = 0;
    
    _texturesAreLoaded = false;
}