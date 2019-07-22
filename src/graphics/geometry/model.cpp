#include "model.h"

#include <string>

void countElements(FILE* file, size_t& numMesh, size_t& numVertex, size_t& numIndex) {
    char lineHeader[512];
    int res = fscanf(file, "%s%*[^\n]", lineHeader);
    // Count objects, vertices and indices.
    while (res != EOF) {
        // read a line and save only the first word

        if (strcmp(lineHeader, "o") == 0) {
            numMesh++;
        } else if (strcmp(lineHeader, "v") == 0) {
            numVertex++;
        } else if (strcmp(lineHeader, "f") == 0) {
            numIndex += 3;
        }
        // Get next header
        res = fscanf(file, "%s%*[^\n]", lineHeader);
    }

    if (numMesh == 0){
        numMesh = 1;
    }

    rewind(file);
}

void fillBuffers(FILE* file, prt::array<Mesh>& meshes,
                 prt::array<Vertex>& vertexBuffer, 
                 prt::array<uint32_t>& indexBuffer) {

    // currently support 2^16 vertices
    constexpr size_t VERTEX_BUFFER_SIZE = UINT16_MAX;

    // Temporary vertex buffer
    Vertex vertexBufferTemp[VERTEX_BUFFER_SIZE];
    // Variables for counting
    size_t meshCount = 0;
    size_t vertexPosCount = 0;
    size_t vertexNormalCount = 0;
    size_t vertexTexCoordCount = 0;
    size_t indicesCount = 0;

    // Parse the contents.
    char lineHeader[512];
    int res = fscanf(file, "%s", lineHeader);
    while (res != EOF) {
        // read the first word of the line
        if (res == EOF) {
            break; // break when we've reached end of file
        }

        // An obj-file with a single object may not have
        // a "o" header.
        if (meshes.size() == 1) {
            meshes[0].numIndices = indexBuffer.size();
            meshes[0].startIndex = 0;
        }

        if (strcmp(lineHeader, "o") == 0) {
            // Object.
            fscanf(file, "%*[^\n]\n", NULL);

            if (meshCount > 0) {
                meshes[meshCount].startIndex = 
                    meshes[meshCount - 1].startIndex + meshes[meshCount - 1].numIndices;
                meshes[meshCount - 1].numIndices = 
                    indicesCount - meshes[meshCount - 1].startIndex;
            } else {
                meshes[meshCount].startIndex = 0;
            }
            // This will be overwritten unless we've reached
            // the final mesh.
            meshes[meshCount].numIndices = 
                indicesCount - indexBuffer.size();

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
            vertexTexCoordCount++;
        } else if (strcmp(lineHeader, "f") == 0) {
            // Parse face.
            std::string vertex1, vertex2, vertex3;
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
            vertexBuffer[vertexIndex[0] - 1].pos = vertexBufferTemp[vertexIndex[0] - 1].pos;
            vertexBuffer[vertexIndex[0] - 1].normal = vertexBufferTemp[normalIndex[0] - 1].normal;
            vertexBuffer[vertexIndex[0] - 1].texCoord = vertexBufferTemp[uvIndex[0] - 1].texCoord;

            vertexBuffer[vertexIndex[1] - 1].pos = vertexBufferTemp[vertexIndex[1] - 1].pos;
            vertexBuffer[vertexIndex[1] - 1].normal = vertexBufferTemp[normalIndex[1] - 1].normal;
            vertexBuffer[vertexIndex[2] - 1].texCoord = vertexBufferTemp[uvIndex[1] - 1].texCoord;

            vertexBuffer[vertexIndex[2] - 1].pos = vertexBufferTemp[vertexIndex[2] - 1].pos;
            vertexBuffer[vertexIndex[2] - 1].normal = vertexBufferTemp[normalIndex[2] - 1].normal;
            vertexBuffer[vertexIndex[2] - 1].texCoord = vertexBufferTemp[uvIndex[2] - 1].texCoord;
            
            indexBuffer[indicesCount++] = vertexIndex[0] - 1;
            indexBuffer[indicesCount++] = vertexIndex[1] - 1;
            indexBuffer[indicesCount++] = vertexIndex[2] - 1;
        }
        res = fscanf(file, "%s", lineHeader);
    }
}

Model::Model(const char* path, Allocator& allocator) {
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", path);
        printf("'!\n");
        assert(false);
        return;
    }

    size_t numMesh = 0;
    size_t numVertex = 0;
    size_t numIndex = 0;
    countElements(file, numMesh, numVertex, numIndex);

    meshes = prt::array<Mesh>(allocator, numMesh, sizeof(size_t));
    vertexBuffer = prt::array<Vertex>(allocator, numVertex, 4 * sizeof(float));
    indexBuffer = prt::array<uint32_t>(allocator, numIndex, sizeof(uint32_t));

    fillBuffers(file, meshes, vertexBuffer, indexBuffer);
}