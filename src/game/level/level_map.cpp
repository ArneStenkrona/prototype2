#include "level_map.h"
#include  "src/util/string_util.h"

#include "src/container/hash_map.h"
#include "src/container/hash_set.h"

#include <fstream>
#define _unused(x) ((void)(x))

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
    _unused(rowLength);
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
    uint32_t texRes = 64;

    model.vertexBuffer.resize(4 * _levelData.size() * _levelData[0].size());
    model.indexBuffer.resize(6 * _levelData.size() * _levelData[0].size());

    uint32_t vi = 0;
    uint32_t ii = 0;
    for (size_t row = 0; row < _levelData.size(); row++) {
        for (size_t col = 0; col < _levelData[0].size(); col++) {
            float texX0 = float(col) / float(_levelData[0].size());
            float texX1 = float(col + 1) / float(_levelData[0].size());
            float texY0 = float(row) / float(_levelData.size());
            float texY1 = float(row + 1) / float(_levelData.size());

            Vertex v1;
            v1.pos = glm::vec3{ float(col), 0.0f, float(row) };
            v1.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v1.texCoord = glm::vec2{ texX0, texY0 };
            Vertex v2;
            v2.pos = glm::vec3{ float(col + 1), 0.0f, float(row) };
            v2.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v2.texCoord = glm::vec2{ texX1, texY0 };
            Vertex v3;
            v3.pos = glm::vec3{ float(col + 1), 0.0f, float(row + 1) };
            v3.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v3.texCoord = glm::vec2{ texX1, texY1 };
            Vertex v4;
            v4.pos = glm::vec3{ float(col), 0.0f, float(row + 1) };
            v4.normal = glm::vec3{ 0.0f, 1.0f, 0.0f };
            v4.texCoord = glm::vec2{ texX0, texY1 };
            
            model.indexBuffer[ii++] = vi+2;
            model.indexBuffer[ii++] = vi+1;
            model.indexBuffer[ii++] = vi;

            model.indexBuffer[ii++] = vi+3;
            model.indexBuffer[ii++] = vi+2;
            model.indexBuffer[ii++] = vi;
            
            model.vertexBuffer[vi++] = v1;
            model.vertexBuffer[vi++] = v2;
            model.vertexBuffer[vi++] = v3;
            model.vertexBuffer[vi++] = v4;
        }
    }

    model.meshes.push_back(Mesh());
    Mesh& mesh = model.meshes.back();
    mesh.startIndex = 0;
    mesh.numIndices = ii;
    mesh.materialIndex = model.materials.size();
    model.materials.push_back({});
    auto & mat = model.materials.back();
    strcpy(mat.fragmentShader, "cel.frag");
    createTexture(mat.texture, texRes);
}

void LevelMap::createTexture(Texture& texture, uint32_t resolution) {
    resolution = resolution > 0 ? resolution : 1;
    texture.texWidth = resolution * _levelData[0].size();
    texture.texHeight = resolution * _levelData.size();
    texture.texChannels = 4;
    texture.pixelBuffer.resize(texture.texChannels * texture.texWidth * texture.texHeight);

    prt::vector<Texture> textures;
    textures.resize(_texturePaths.size());
    for (uint32_t i = 0; i < textures.size(); i++) {
        textures[i].load(_texturePaths[i].c_str());
    }

    for (size_t row = 0; row < _levelData.size(); row++) {
        for (size_t col = 0; col < _levelData[0].size(); col++) {
            for (size_t y = 0; y < resolution; y++) {
                for (size_t x = 0; x < resolution; x++) {
                    auto yOffset = texture.texChannels * resolution * resolution * row * _levelData[0].size() +
                                   texture.texChannels * resolution * _levelData[0].size() * y;

                    auto xOffset = texture.texChannels * resolution * col +
                                   texture.texChannels * x;

                    size_t bottom = row > 0 ? row - 1 : row;
                    size_t top = row + 1 < _levelData.size() ? row + 1 : row;
                    size_t left = col + 1 < _levelData[0].size() ? col + 1 : col;
                    size_t right = col > 0 ? col - 1 : col;
                    LevelIndex qx = x > resolution / 2 ? LevelIndex{row, left} : LevelIndex{row, right};
                    LevelIndex qy = y > resolution / 2 ? LevelIndex{top, col} : LevelIndex{bottom, col};
                    LevelIndex qxy = LevelIndex{qy.row, qx.col};

                    float center = float(resolution - 1) / float(2);
                    uint32_t val = _levelData[row][col];
                    uint32_t xVal = _levelData[qx.row][qx.col];
                    uint32_t yVal = _levelData[qy.row][qy.col];
                    uint32_t xyVal = _levelData[qxy.row][qxy.col];
                    float xDist = 2 * float(x > resolution / 2 ? 1.0f : -1.0f) * (float(x) - center) / float(resolution - 1);
                    xDist =  std::pow(std::max(0.0f, xDist), 1.2f);
                    float yDist = 2 * float(y > resolution / 2 ? 1.0f : -1.0f) * (float(y) - center) / float(resolution - 1);
                    yDist = std::pow(std::max(0.0f, yDist), 1.2f);

                    float xf = val == xVal ? 1.0f : 0.0f;
                    float yf = val == yVal ? 1.0f : 0.0f;
                    float xyf = val == xyVal ? 1.0f : 0.0f;

                    float fc = (1.0f - xDist) * (1.0f - yDist) + xf * xDist * (1.0f - yDist) + yf * (1.0f - xDist) * yDist + xyf * xf * yf * xDist * yDist;

                    float sx = float(x) / float(resolution - 1);
                    float sy = float(y) / float(resolution - 1);
                    unsigned char* color = fc > 0.3f ? textures[val].sample(sx,sy) : textures[0].sample(sx,sy);
                    
                    texture.pixelBuffer[yOffset + xOffset]     = color[0];
                    texture.pixelBuffer[yOffset + xOffset + 1] = color[1];
                    texture.pixelBuffer[yOffset + xOffset + 2] = color[2];
                    texture.pixelBuffer[yOffset + xOffset + 3] = color[3];
                }
            }
        }
    }
}