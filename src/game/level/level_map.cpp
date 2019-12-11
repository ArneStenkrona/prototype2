#include "level_map.h"
#include  "src/util/string_util.h"

#include "src/container/hash_map.h"
#include "src/container/hash_set.h"

#include <fstream>

LevelMap::LevelMap(const char* filePath) {

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
    //prt::vector<Vertex> vertices;
    //prt::vector<uint32_t> indices;

    //prt::hash_map<LevelIndex, prt::vector<LevelIndex>>
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
}