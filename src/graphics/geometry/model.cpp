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
            size_t matIndex = materials.size();
            while (assignedMaterials < meshCount) {
                meshes[assignedMaterials++].materialIndex = matIndex;
            }
            materials.push_back({});
            auto & mat = materials.back();
            strcpy(mat.name, currentMtl);
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

            for (size_t i = 0; i < 3; ++i) {
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
        auto & material = materials[mesh.materialIndex];
        if (mtlToFragShader.find(material.name) == mtlToFragShader.end()) {
            strcpy(material.fragmentShader, "cel.frag");
        } else {
            strcpy(material.fragmentShader, mtlToFragShader[material.name].c_str());
        }

        char texturePath[256];
        strcpy(texturePath, path);
        ptr = strrchr(texturePath, '/');
        strcpy(++ptr, mtlToTexture[material.name].c_str());
        material.texture.load(texturePath);
    }
}   

void Model::parseMeshFBX(FBX_Document::FBX_Node const & node) {
    meshes.push_back({});
    Mesh & mesh = meshes.back();

    // Retrieve name
    char *name = reinterpret_cast<char*>(node.getProperty(1).data.data());
    strcpy(mesh.name, name);

    // Retrieve vertices
    prt::vector<unsigned char> const & vertices = node.find("Vertices")->getProperty(0).data;
    prt::vector<unsigned char> const & indices = node.find("PolygonVertexIndex")->getProperty(0).data;

    // Retrieve normals
    const FBX_Document::FBX_Node *layerNormal = node.find("LayerElementNormal");
    const char *normalMap = reinterpret_cast<char*>(layerNormal->find("MappingInformationType")->getProperty(0).data.data());
    if (strcmp(normalMap, "ByPolygonVertex") != 0) {
        assert(false && "Mapping Information Type for normal has to be By Polygon Vertex!");
        return;
    }
    const char *normalRef = reinterpret_cast<char*>(layerNormal->find("ReferenceInformationType")->getProperty(0).data.data());
    if (strcmp(normalRef, "Direct") != 0) {
        assert(false && "Reference Information Type for normal has to be Direct!");
        return;
    }
    prt::vector<unsigned char> const & normals = layerNormal->find("Normals")->getProperty(0).data;

    // Retrieve uv coordinates
    const FBX_Document::FBX_Node *layerUV = node.find("LayerElementUV");
    const char *uvMap = reinterpret_cast<char*>(layerUV->find("MappingInformationType")->getProperty(0).data.data());
    if (strcmp(uvMap, "ByPolygonVertex") != 0) {
        assert(false && "Mapping Information Type for UV has to be By Polygon Vertex!");
        return;
    }
    const char *uvRef = reinterpret_cast<char*>(layerUV->find("ReferenceInformationType")->getProperty(0).data.data());
    if (strcmp(uvRef, "IndexToDirect") != 0) {
        assert(false && "Reference Information Type for UV has to be Index To Direct!");
        return;
    }
    prt::vector<unsigned char> const & uvs = layerUV->find("UV")->getProperty(0).data;
    prt::vector<unsigned char> const & uvIndices = layerUV->find("UVIndex")->getProperty(0).data;

    // Retrieve material, if any
    const FBX_Document::FBX_Node *layerMat = node.find("LayerElementMaterial");
    if (layerMat != nullptr) {
        const char *matMap = reinterpret_cast<char*>(layerMat->find("MappingInformationType")->getProperty(0).data.data());
        if (strcmp(matMap, "AllSame") != 0) {
            assert(false && "Mapping Information Type for materials has to be All Same!");
            return;
        }
        const char *matRef = reinterpret_cast<char*>(layerMat->find("ReferenceInformationType")->getProperty(0).data.data());
        if (strcmp(matRef, "IndexToDirect") != 0) {
            assert(false && "Reference Information Type for materials has to be Index To Direct!");
            return;
        }
        prt::vector<unsigned char> const & materials = layerMat->find("Materials")->getProperty(0).data;
        mesh.materialIndex = *reinterpret_cast<int32_t*>(materials.data());
    }

    prt::vector<Vertex> vertexBufferTemp;
    vertexBufferTemp.resize(indices.size() / sizeof(int32_t));
    prt::hash_map<Vertex, uint32_t> uniqueVertices;

    double* vert = reinterpret_cast<double*>(vertices.data());
    size_t vi = 0;
    for (size_t i = 0; i < vertices.size() / sizeof(double); i += 3) {
        new (&vertexBufferTemp[vi].pos.x) float(vert[i]);
        new (&vertexBufferTemp[vi].pos.y) float(vert[i+1]);
        new (&vertexBufferTemp[vi].pos.z) float(vert[i+2]);
        ++vi;
    }
    double* norm = reinterpret_cast<double*>(normals.data());
    vi = 0;
    for (size_t i = 0; i < normals.size() / sizeof(double); i += 3) {
        new (&vertexBufferTemp[vi].normal.x) float(norm[i]);
        new (&vertexBufferTemp[vi].normal.y) float(norm[i+1]);
        new (&vertexBufferTemp[vi].normal.z) float(norm[i+2]);
        ++vi;
    }
    double* texc = reinterpret_cast<double*>(uvs.data());
    vi = 0;
    for (size_t i = 0; i < uvs.size() / sizeof(double); i += 2) {
        new (&vertexBufferTemp[vi].texCoord.x) float(texc[i]);
        new (&vertexBufferTemp[vi].texCoord.y) float(texc[i+1]);
        ++vi;
    }

    size_t indexCount = indexBuffer.size();
    size_t vertexCount = vertexBuffer.size();
    indexBuffer.resize(indexBuffer.size() + indices.size() / sizeof(uint32_t));
    int32_t* ind = reinterpret_cast<int32_t*>(indices.data());
    int32_t* uvInd = reinterpret_cast<int32_t*>(uvIndices.data());  

    for (size_t i = 0; i < indices.size() / sizeof(int32_t); i += 3) {
        assert((ind[i+2] < 0) && "Face is not triangular!");
        uint32_t i0 = uint32_t(ind[i]);
        uint32_t i1 = uint32_t(ind[i+1]);
        uint32_t i2 = uint32_t(-ind[i+2] - 1);

        Vertex v[3] =  {
            { vertexBufferTemp[i0].pos,
              vertexBufferTemp[i].normal,
              vertexBufferTemp[uvInd[i0]].texCoord },
            { vertexBufferTemp[i1].pos,
              vertexBufferTemp[i+1].normal,
              vertexBufferTemp[uvInd[i1]].texCoord },
            { vertexBufferTemp[i2].pos,
              vertexBufferTemp[i+2].normal,
              vertexBufferTemp[uvInd[i2]].texCoord },
        };
        for (size_t i = 0; i < 3; ++i) {
            if (uniqueVertices.find(v[i]) == uniqueVertices.end()) {
                uniqueVertices[v[i]] = static_cast<uint32_t>(vertexCount++);
            }
            indexBuffer[indexCount++] = uniqueVertices[v[i]];
        }  
    }
    vertexBuffer.resize(vertexCount);
    for (auto const & entry : uniqueVertices) {
        vertexBuffer[entry.value()] = entry.key();
    }
}

void Model::parseMaterialFBX(FBX_Document::FBX_Node const & /*node*/) {

}

void Model::parseTextureFBX(FBX_Document::FBX_Node const & /*node*/) {
    // const char *filename = reinterpret_cast<const char*>(node.find("RelativeFileName")->getProperty(0).data.data());
}

void Model::loadFBX(const char* path) {
    FBX_Document doc(path);
    auto const & objects = doc.getNode("Objects");
    assert(objects && "FBX file must contain Objects node!");
    auto const & children = objects->getChildren();

    for (auto const & child : children) {
        if (strcmp(child.getName(), "Geometry") == 0) {
            const char *type = reinterpret_cast<const char*>(child.getProperty(2).data.data());
            if (strcmp(type, "Mesh") == 0) {
                parseMeshFBX(child);
            } else if (strcmp(type, "Material") == 0) {
                parseMaterialFBX(child);
            } else if (strcmp(type, "Texture") == 0) {
                parseTextureFBX(child);
            }
        }
    }
    
}