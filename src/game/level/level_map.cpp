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
                std::cout << neigh.row << ", " << neigh.col << std::endl;
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


void LevelMap::getModels(prt::vector<Model>& models) {
    models.resize(0);

    prt::hash_set<LevelIndex> inIsland;
    prt::vector<prt::vector<LevelIndex> > islands;
    
    for (size_t i = 0; i < _levelData.size(); i++) {
        for (size_t j = 0; j < _levelData.size(); j++) {
            if (inIsland.find({i, j}) == inIsland.end()) {
                prt::vector<LevelIndex> island;
                findIsland({i, j}, island);
                islands.push_back(island);
                for (size_t k = 0; k < island.size(); k++) {
                    inIsland.insert(island[k]);
                }
            }
        }
    }

    for (size_t isl = 0; isl < islands.size(); isl++) {
        prt::vector<prt::vector<char> > prints;
        prints.resize(_levelData.size());
        for (size_t i = 0; i < prints.size(); i++) {
            prints[i].resize(_levelData[0].size(), '-');
        }

        std::cout << "_______________________" << std::endl;
        for (size_t i = 0; i < islands[isl].size(); i++) {
            LevelIndex& idx = islands[isl][i];
            prints[idx.row][idx.col] = '#';
        }

        for (size_t i = 0; i < prints.size(); i++) {
            for (size_t j = 0; j < prints[i].size(); j++) {
                std::cout << prints[i][j];
            }
            std::cout << std::endl;
        }
    }
    for (size_t isl = 0; isl < islands.size(); isl++) {
        prt::vector<LevelIndex>& island = islands[isl];
        models.push_back(Model());
        Model& mdl = models.back();

        mdl._vertexBuffer.resize(4 * island.size());
        mdl._indexBuffer.resize(6 * island.size());
        uint32_t vi = 0;
        uint32_t ii = 0;
        for (size_t i = 0; i < island.size(); i++) {
            size_t& row = island[i].row;
            size_t& col = island[i].col;

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
            
            mdl._indexBuffer[ii++] = vi+2;
            mdl._indexBuffer[ii++] = vi+1;
            mdl._indexBuffer[ii++] = vi;

            mdl._indexBuffer[ii++] = vi+3;
            mdl._indexBuffer[ii++] = vi+2;
            mdl._indexBuffer[ii++] = vi;
            
            mdl._vertexBuffer[vi++] = v1;
            mdl._vertexBuffer[vi++] = v2;
            mdl._vertexBuffer[vi++] = v3;
            mdl._vertexBuffer[vi++] = v4;

        }
        Mesh mesh;
        mesh.startIndex = 0;
        mesh.numIndices = mdl._indexBuffer.size();
        mdl._meshes.push_back(mesh);
        uint32_t value = _levelData[island[0].row][island[0].col];
        std::string texturePath = value < _texturePaths.size() ? _texturePaths[value] : _texturePaths[0];
        mdl._texture.load(texturePath.c_str());
    }
}