#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include "src/container/hash_map.h"

#include <dirent.h>
#include <fstream>

#include <string>   

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

ModelManager::ModelManager(const char* modelDirectory)
    : _modelDirectory(modelDirectory) {
    loadOBJPaths(modelDirectory);
}

void ModelManager::loadOBJPaths(const char* directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return;
    }
    // Add default
    std::string modelAssetPath = directory;
    _modelPaths.insert("DEFAULT", modelAssetPath + "DEFAULT/model.obj");
    _idToName.insert(nextID, "DEFAULT");
    _modelIDs.insert("DEFAULT", nextID++);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp (entry->d_name, "MODEL_", strlen("MODEL_")) == 0) {
            auto modelDir = std::string(entry->d_name);
            std::string modelName = modelDir.substr(strlen("MODEL_"));
            assert((modelName.compare("DEFAULT") != 0));
            _modelPaths.insert(modelName, modelAssetPath + modelDir);
            _idToName.insert(nextID, modelName);
            _modelIDs.insert(modelName, nextID++);
        }
    }

    closedir(dir);
}

void ModelManager::getPaths(const prt::vector<uint32_t>& uniqueIDs, 
              prt::vector<std::string>& modelPaths) {
    modelPaths.resize(uniqueIDs.size());
    for (size_t i = 0; i < uniqueIDs.size(); i++) {
        std::string& name = _idToName[uniqueIDs[i]];
        modelPaths[i] = _modelPaths[name];
    }
}

uint32_t ModelManager::getModelID(std::string& name) {
    auto it = _modelIDs.find(name);
    assert((it != _modelIDs.end()));
    return it->value();
}

uint32_t ModelManager::getModelID(const char* name) {
    std::string name_str = std::string(name);
    return getModelID(name_str);
}

void ModelManager::loadModels(const prt::vector<uint32_t>& uniqueIDs, prt::vector<Model>& models) {
    prt::vector<std::string> modelPaths;
    getPaths(uniqueIDs, modelPaths);
    models.resize(uniqueIDs.size());
    for (uint32_t i = 0; i < modelPaths.size(); i++) {    
        loadOBJ((modelPaths[i] + "/model.obj").c_str(), models[i]);

        for (size_t mi = 0; mi < models[i]._meshes.size(); mi++) {
            std::string texPath =  modelPaths[i] + "/" + models[i]._meshes[mi]._name + "_diffuse.png";
            models[i]._meshes[mi]._texture.load(texPath.c_str());
        }
    }
}

void ModelManager::loadSceneModels(const prt::vector<uint32_t>& modelIDs, 
                                   prt::vector<Model>& models, 
                                   prt::vector<uint32_t>& modelIndices) {
    modelIndices.resize(modelIDs.size());                                  
    prt::hash_map<uint32_t, uint32_t> idToIndex;
    prt::vector<uint32_t> uniqueIDs;
    for (size_t i = 0; i < modelIDs.size(); i++) {
        const uint32_t& indx = modelIDs[i];
        if (idToIndex.find(indx) == idToIndex.end()) {
            idToIndex.insert(indx, uniqueIDs.size());
            uniqueIDs.push_back(indx);
        }
        modelIndices[i] = idToIndex[indx];
    }
    loadModels(uniqueIDs, models);
}

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

void ModelManager::loadOBJ(const char* modelPath, Model& model) {
    prt::vector<Mesh>& meshes = model._meshes;
    prt::vector<Vertex>& vertexBuffer = model._vertexBuffer;
    prt::vector<uint32_t>& indexBuffer = model._indexBuffer;

    FILE* file = fopen(modelPath, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", modelPath);
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
            // object name
            fscanf(file, "%s", meshes[meshCount]._name);
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
            meshes[meshCount].numIndices = indexBuffer.size() - indexCount;

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
                indexBuffer[indexCount++] = uniqueVertices[vertices[i]];
            }        
        }
     
        res = fscanf(file, "%s", lineHeader);
    }
    size_t numVertex = uniqueVertices.size();
    vertexBuffer.resize(numVertex);
    
    indexCount = 0;
    for (auto it = uniqueVertices.begin(); it != uniqueVertices.end(); it++) {
        vertexBuffer[it->value()] = it->key();
    }
}