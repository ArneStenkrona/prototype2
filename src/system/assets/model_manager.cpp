#include "model_manager.h"

#include "src/system/assets/asset_manager.h"

#include "src/util/string_util.h"

#include <dirent.h>
#include <fstream>

#include <string>   

#define STB_IMAGE_IMPLEMENTATION
#include <stb-master/stb_image.h>

bool is_file_exist(const char *fileName)
{
    std::ifstream infile(fileName);
    return infile.good();
}

ModelManager::ModelManager(const char* directory)
    : _directory(directory) {
    loadPersistent(directory);
    parametric_shapes::Quad quad;
    quad.width = 20;
    quad.height = 150;
    quad.resW = 20;
    quad.resH = 150;
    prt::vector<parametric_shapes::Quad> quads = { quad };
    insertQuads(quads);
    parametric_shapes::Sphere sphere;
    sphere.radius = 10.0f;
    sphere.res = 50;
    prt::vector<parametric_shapes::Sphere> spheres = { sphere };
    insertSpheres(spheres);
    parametric_shapes::Cylinder cylinder;
    cylinder.radius = 10.0f;
    cylinder.height = 20.0f;
    cylinder.res = 50;
    prt::vector<parametric_shapes::Cylinder> cylinders = { cylinder };
    insertCylinders(cylinders);
    parametric_shapes::Capsule capsule;
    capsule.radius = 10.0f;
    capsule.height = 40.0f;
    capsule.res = 50;
    prt::vector<parametric_shapes::Capsule> capsules = { capsule };
    insertCapsules(capsules);
}

void ModelManager::loadPersistent(const char* directory) {
    struct dirent *entry;
    DIR *dir = opendir(directory);
    if (dir == NULL) {
        return;
    }
    // Add default
    std::string modelAssetPath = std::string(AssetManager::persistentStorageString) + directory;
    _modelPaths.insert("DEFAULT", modelAssetPath + "DEFAULT/model.obj");
    _texturePaths.insert("DEFAULT", modelAssetPath + "DEFAULT/diffuse.png");
    _modelIDs.insert("DEFAULT", nextID++);
    while ((entry = readdir(dir)) != NULL) {
        if (strncmp (entry->d_name, "MODEL_", strlen("MODEL_")) == 0) {
            auto modelDir = std::string(entry->d_name);
            std::string modelName = modelDir.substr(strlen("MODEL_"));
            assert((modelName.compare("DEFAULT") != 0));
            _modelPaths.insert(modelName, modelAssetPath + modelDir + "/model.obj");
            auto texturePath = is_file_exist((directory + modelDir + "/diffuse.png").c_str()) ?
                               modelAssetPath + modelDir + "/diffuse.png" : modelAssetPath + "DEFAULT/diffuse.png";
            _texturePaths.insert(modelName, texturePath);
            _modelIDs.insert(modelName, nextID++);
        }
    }

    closedir(dir);
}

void ModelManager::getPaths(prt::vector<std::string>& modelPaths, prt::vector<std::string>& texturePaths) {
    modelPaths.resize(_modelPaths.size());
    for (auto it = _modelPaths.begin(); it != _modelPaths.end(); it++) {
        uint32_t index = _modelIDs[it->key()];
        modelPaths[index] = it->value();
    }
    texturePaths.resize(_texturePaths.size());
    for (auto it = _texturePaths.begin(); it != _texturePaths.end(); it++) {
        uint32_t index = _modelIDs[it->key()];
        texturePaths[index] = it->value();
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

void ModelManager::loadModels(prt::vector<Model>& models) {
    prt::vector<std::string> modelPaths;
    prt::vector<std::string> texturePaths;
    getPaths(modelPaths, texturePaths);
    assert((modelPaths.size() == texturePaths.size()));
    models.resize(nextID);
    for (uint32_t i = 0; i < modelPaths.size(); i++) {
        std::string modelStorage = modelPaths[i].substr(0, modelPaths[i].find("/"));
    
        if (modelStorage.compare(AssetManager::persistentStorageString) == 0) { // persistent storage
            std::string modelPath = modelPaths[i].substr(modelPaths[i].find(AssetManager::persistentStorageString) + 2);
            loadMeshes(modelPath.c_str(), models[i]._meshes, models[i]._vertexBuffer, models[i]._indexBuffer);
        } else if (modelStorage.compare(AssetManager::NonPersistentStorageString) == 0) { //  non-persistent storage
            std::string modelPath = modelPaths[i].substr(modelPaths[i].find(AssetManager::NonPersistentStorageString) + 2);
            prt::vector<std::string> split = string_util::splitString(modelPath, '/');
            std::string& type = split[1];
            uint32_t index = std::stoi(split[2]);
            if (type.compare("QUAD") == 0) {
                parametric_shapes::createQuad(models[i]._vertexBuffer, models[i]._indexBuffer, _quads[index]);
            } else if (type.compare("CUBOID") == 0) {
            } else if (type.compare("SPHERE") == 0) {
                parametric_shapes::createSphere(models[i]._vertexBuffer, models[i]._indexBuffer, _spheres[index]);
            } else if (type.compare("CYLINDER") == 0) {
                parametric_shapes::createCylinder(models[i]._vertexBuffer, models[i]._indexBuffer, _cylinders[index]);
            } else if (type.compare("CAPSULE") == 0) {
                parametric_shapes::createCapsule(models[i]._vertexBuffer, models[i]._indexBuffer, _capsules[index]);
            }
            Mesh mesh;
            mesh.startIndex = 0;
            mesh.numIndices = models[i]._indexBuffer.size();
            models[i]._meshes.push_back(mesh);
        }

        std::string textureStorage = texturePaths[i].substr(0, texturePaths[i].find("/"));
    
        if (textureStorage.compare(AssetManager::persistentStorageString) == 0) { // persistent storage
            std::string texturePath = texturePaths[i].substr(texturePaths[i].find(AssetManager::persistentStorageString) + 2);
            loadTextures(texturePath.c_str(), models[i]._texture);
        } else if (textureStorage.compare(AssetManager::NonPersistentStorageString) == 0) { //  non-persistent storage
            // TODO: add loading for non-persistent data
        }
    }

 
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

void ModelManager::loadMeshes(const char* modelPath, prt::vector<Mesh>& meshes, 
                                                    prt::vector<Vertex>& vertexBuffer,
                                                    prt::vector<uint32_t>& indexBuffer) {

    //std::string modelPath = _modelPath;

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

void ModelManager::loadTextures(const char* texturePath, Texture& texture) {
    int texWidth, texHeight, texChannels;

    stbi_uc* pixels = stbi_load(texturePath, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
    if (!pixels) {
        throw std::runtime_error("failed to load texture image!");
    }

    texture.texWidth = texWidth;
    texture.texHeight = texHeight;
    texture.texChannels = texChannels;

    size_t bufferSize = texWidth * texHeight * 4;
    texture.pixelBuffer.resize(bufferSize);
    for (size_t i = 0; i < texture.pixelBuffer.size(); i++) {
        texture.pixelBuffer[i] = pixels[i];
    }

    stbi_image_free(pixels);
}