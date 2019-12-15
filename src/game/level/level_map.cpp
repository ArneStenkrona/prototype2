#include "level_map.h"
#include  "src/util/string_util.h"

#include "src/container/hash_map.h"
#include "src/container/hash_set.h"

#include <fstream>

LevelMap::LevelMap(const char* path) {
    std::string levelDataPath = std::string(path) + "level.lvl";
    loadLevelData(levelDataPath.c_str());

    _texturePaths.push_back(std::string(path) + "0.png");
    _texturePaths.push_back(std::string(path) + "1.png");
}

void LevelMap::loadLevelData(const char* filePath) {
    FILE* file = fopen(filePath, "r");
    if (file == nullptr) {
        printf("Could not open file '");
        printf("%s", filePath);
        printf("'!\n");
        assert(false);
        return;
    }
    // Parse the contents.
    char lineHeader[512];
    char* res = fgets(lineHeader, 512, file);
    size_t rowLength = string_util::splitString(std::string(lineHeader), ',').size();
    while (res != NULL) {
        prt::vector<std::string> split = string_util::splitString(std::string(lineHeader), ',');

        assert((split.size() == rowLength) && "All rows should be equal length");

        prt::vector<uint32_t> row;
        row.resize(split.size());
        for (size_t i = 0; i < split.size(); i++) {
            row[i] = static_cast<uint32_t>(std::stoul(split[i]));
        }
        _levelData.push_back(row);
        res = fgets(lineHeader, 512, file);
    }
}

void LevelMap::findIsland(LevelIndex lvlIndex, prt::vector<LevelIndex>& island) {
    island.resize(0);
    island.push_back(lvlIndex);
    prt::hash_set<LevelIndex> visited;
    visited.insert(lvlIndex);

    uint32_t value = _levelData[lvlIndex.row][lvlIndex.col];

    prt::vector<LevelIndex> neighbours;
    prt::vector<LevelIndex> stack;
    stack.push_back(lvlIndex);
    while (!stack.empty()) {
        LevelIndex idx = stack.back();
        stack.pop_back();
        find4Neighbours(idx, neighbours);
        for (size_t i = 0; i < neighbours.size(); i++)  {
            LevelIndex& neigh = neighbours[i];
            if (visited.find(neigh) == visited.end()) {
                if (_levelData[neigh.row][neigh.col] == value) {
                    island.push_back(neigh);
                    stack.push_back(neigh);
                }
                visited.insert(neigh);
            } 
        }
    }
}

void LevelMap::find4Neighbours(LevelIndex lvlIndex, prt::vector<LevelIndex>& neighbours) {
    neighbours.resize(0);
    size_t& row = lvlIndex.row;
    size_t& col = lvlIndex.col;

    if (row > 0) {
        neighbours.push_back({row - 1, col});
    }
    if (col > 0) {
        neighbours.push_back({row, col - 1});
    }
    if (row + 1 < _levelData.size()) {
        neighbours.push_back({row + 1, col});
    }
    if (col + 1 < _levelData[0].size()) {
        neighbours.push_back({row, col + 1});
    }
}

void LevelMap::loadModel(Model& model) {
    model._vertexBuffer.resize(4 * _levelData.size() * _levelData[0].size());
    model._indexBuffer.resize(6 * _levelData.size() * _levelData[0].size());

    prt::hash_map<uint32_t, uint32_t> valueToMesh;
    prt::vector<prt::vector<LevelIndex> > indexPerMesh;
    uint32_t nMeshes = 0;
    for (size_t row = 0; row < _levelData.size(); row++) {
        for (size_t col = 0; col < _levelData[0].size(); col++) {
            uint32_t val = _levelData[row][col];
            if (valueToMesh.find(val) == valueToMesh.end()) {
                valueToMesh.insert(val, nMeshes++);
                indexPerMesh.resize(nMeshes);
            }
            uint32_t meshIndx = valueToMesh.find(val)->value();
            indexPerMesh[meshIndx].push_back({row, col});
        }
    }

    model._meshes.resize(nMeshes);
    uint32_t vi = 0;
    uint32_t ii = 0;
    uint32_t indexOffset = 0;
    for (size_t i = 0; i < indexPerMesh.size(); i++) {
        for (size_t j = 0; j < indexPerMesh[i].size(); j++) {
            size_t& row = indexPerMesh[i][j].row;
            size_t& col = indexPerMesh[i][j].col;

            Vertex v1;
            v1.pos = glm::vec3{ float(row), 0.0f, float(col) };
            v1.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v1.texCoord = glm::vec2{ 0.0f, 0.0f };
            Vertex v2;
            v2.pos = glm::vec3{ float(row + 1), 0.0f, float(col) };
            v2.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v2.texCoord = glm::vec2{ 1.0f, 0.0f };
            Vertex v3;
            v3.pos = glm::vec3{ float(row + 1), 0.0f, float(col + 1) };
            v3.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v3.texCoord = glm::vec2{ 1.0f, 1.0f };
            Vertex v4;
            v4.pos = glm::vec3{ float(row), 0.0f, float(col + 1) };
            v4.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v4.texCoord = glm::vec2{ 0.0f, 1.0f };
            
            model._indexBuffer[ii++] = vi+2;
            model._indexBuffer[ii++] = vi+1;
            model._indexBuffer[ii++] = vi;

            model._indexBuffer[ii++] = vi+3;
            model._indexBuffer[ii++] = vi+2;
            model._indexBuffer[ii++] = vi;
            
            model._vertexBuffer[vi++] = v1;
            model._vertexBuffer[vi++] = v2;
            model._vertexBuffer[vi++] = v3;
            model._vertexBuffer[vi++] = v4;
        }
        Mesh& mesh = model._meshes[i];
        mesh.startIndex = indexOffset;
        mesh.numIndices = ii;
        indexOffset += ii;
        //uint32_t value = _levelData[indexPerMesh[i][0].row][indexPerMesh[i][0].col];
        //std::string texturePath = value < _texturePaths.size() ? _texturePaths[value] : _texturePaths[0];
        createTexture(mesh._texture, 1);
    }
}

void LevelMap::createTexture(Texture& texture, uint32_t resolution) {
    resolution = resolution > 0 ? resolution : 1;
    texture.texWidth = resolution * _levelData[0].size();
    texture.texHeight = resolution * _levelData.size();
    texture.texChannels = 4;
    texture.pixelBuffer.resize(texture.texChannels * texture.texWidth * texture.texHeight);

    for (size_t row = 0; row < _levelData.size(); row++) {
        for (size_t col = 0; col < _levelData[0].size(); col++) {
            for (size_t y = 0; y < resolution; y++) {
                for (size_t x = 0; x < resolution; x++) {
                    auto lvlIndexOffset = texture.texChannels * resolution * row * _levelData[0].size() +
                                          texture.texChannels * resolution * col;

                    auto pixelOffset = texture.texChannels * resolution * y +
                                       texture.texChannels * x;

                    char r = 0xff;
                    char g = 0x00;
                    char b = 0x00;
                    char a = 0xff;
                    texture.pixelBuffer[lvlIndexOffset + pixelOffset] = r;
                    texture.pixelBuffer[lvlIndexOffset + pixelOffset + 1] = g;
                    texture.pixelBuffer[lvlIndexOffset + pixelOffset + 2] = b;
                    texture.pixelBuffer[lvlIndexOffset + pixelOffset + 3] = a;
                }
            }
        }
    }
}