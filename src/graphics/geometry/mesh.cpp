#include "mesh.h"

#include <string>

void Mesh::loadModel(char* path, Allocator& allocator) {
    FILE* file = fopen(path, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", path);
        printf("'!\n");
        assert(false);
        return;
    }

    // Count the vertices and indicies in the file.
    while(true) {
        char lineHeader[512];
        // read a line and save only the first word
        int res = fscanf(file, "%s%*[^\n]", lineHeader);
        if (res == EOF) {
            break; // break when we've reached end of file
        }

        if (strcmp(lineHeader, "v") == 0) {
            // Found a vertex.
            verticesSize++;
        } else if (strcmp(lineHeader, "f") == 0) {
            // Found a face. Assume all faces are triangles
           indicesSize += 3;
        }
    }

    // Allocate
    vertices = static_cast<Vertex* > 
                        (allocator
                        .allocate(verticesSize * sizeof(Vertex), 
                                  4 * sizeof(float)));
    
    indices = static_cast<uint32_t* > 
                    (allocator
                    .allocate(indicesSize * sizeof(uint32_t), 
                              sizeof(uint32_t)));

    // Rewind the file.
    rewind(file);

    // Parse the contents.
    while(true) {
        char lineHeader[512];
        // read the first word of the line
        int res = fscanf(file, "%s", lineHeader);
        if (res == EOF) {
            break; // break when we've reached end of file
        }

        // currently support 2^16 vertices
        constexpr size_t VERTEX_BUFFER_SIZE = UINT16_MAX;

        Vertex vertexBuffer[VERTEX_BUFFER_SIZE];

        size_t vertexPosCount = 0;
        size_t vertexNormalCount = 0;
        size_t vertexTexCoordCount = 0;
        size_t indicesCount = 0;

        if (strcmp(lineHeader, "v") == 0) {
            // Parse vertex position.
            glm::vec3& pos = vertexBuffer[vertexPosCount].pos;
            fscanf(file, "%f %f %f\n", &pos.x, &pos.y, &pos.z );
            vertexPosCount++;
        } else if (strcmp(lineHeader, "vn") == 0) {
            // Parse vertex normal.
            glm::vec3& normal = vertexBuffer[vertexPosCount].normal;
            fscanf(file, "%f %f %f\n", &normal.x, &normal.y, &normal.z );
            vertexNormalCount++;
        } else if (strcmp(lineHeader, "vt") == 0) {
            // Parse vertex texture coordinate.
            glm::vec2& texCoord = vertexBuffer[vertexTexCoordCount].texCoord;
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

            vertices[vertexIndex[0]].pos = vertexBuffer[vertexIndex[0]].pos;
            vertices[vertexIndex[0]].normal = vertexBuffer[normalIndex[0]].normal;
            vertices[vertexIndex[0]].texCoord = vertexBuffer[uvIndex[0]].texCoord;

            vertices[vertexIndex[1]].pos = vertexBuffer[vertexIndex[1]].pos;
            vertices[vertexIndex[1]].normal = vertexBuffer[normalIndex[1]].normal;
            vertices[vertexIndex[2]].texCoord = vertexBuffer[uvIndex[1]].texCoord;

            vertices[vertexIndex[2]].pos = vertexBuffer[vertexIndex[2]].pos;
            vertices[vertexIndex[2]].normal = vertexBuffer[normalIndex[2]].normal;
            vertices[vertexIndex[2]].texCoord = vertexBuffer[uvIndex[2]].texCoord;

            indices[indicesCount++] = vertexIndex[0];
            indices[indicesCount++] = vertexIndex[1];
            indices[indicesCount++] = vertexIndex[2];
        }
    }
}