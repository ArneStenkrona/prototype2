#include "model.h"

#include "src/container/hash_table.h"

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

Model::Model()
: vertexBuffer(4 * sizeof(float)),
  indexBuffer(sizeof(uint32_t)),
  meshes(sizeof(size_t)) {}

void Model::load(const char* path) {

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
    //std::unordered_map<Vertex, uint32_t> uniqueVertices = {}; 
    prt::HashTable<Vertex, uint32_t> uniqueVertices; 

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
    if (meshes.size() == 1) {
        meshes[0].numIndices = indexBuffer.size();
        meshes[0].startIndex = 0;
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
                meshes[meshCount].startIndex = 
                    meshes[meshCount - 1].startIndex + meshes[meshCount - 1].numIndices;
                meshes[meshCount - 1].numIndices = 
                    indexCount - meshes[meshCount - 1].startIndex;                
            } else {
                meshes[meshCount].startIndex = 0;
            }
            // This will be overwritten unless we've reached
            // the final mesh.
            meshes[meshCount].numIndices = 
                indexCount - indexBuffer.size();

            meshCount++;

        } else if (strcmp(lineHeader, "v") == 0) {
            // Parse vertex position.
            glm::vec3& pos = vertexBufferTemp[vertexPosCount].pos;
            fscanf(file, "%f %f %f\n", &pos.x, &pos.y, &pos.z );
            vertexPosCount++;
        } else if (strcmp(lineHeader, "vn") == 0) {
            // Parse vertex normal.
            glm::vec3& normal = vertexBufferTemp[vertexPosCount].normal;
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
                if (uniqueVertices.find(vertices[i]) != uniqueVertices.end()) {
                    uniqueVertices[vertices[i]] = static_cast<uint32_t>(vertexCount++);
                }
                indexBuffer[indexCount++] = uniqueVertices[vertices[i]];
            }        
        }
     
        res = fscanf(file, "%s", lineHeader);
    }
    size_t numVertex = uniqueVertices.size();
    vertexBuffer.resize(numVertex);
    
    indexCount = 0;
    for (auto it = uniqueVertices.begin(); it < uniqueVertices.end(); it++) {
        vertexBuffer[ind] = it->value;
    }
}