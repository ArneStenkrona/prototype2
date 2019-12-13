#ifndef LEVEL_MAP_H
#define LEVEL_MAP_H
#include "src/container/vector.h"

#include "src/graphics/geometry/model.h"

struct LevelIndex {
    size_t row;
    size_t col;

    bool operator==(const LevelIndex& other) const {
        return row == other.row && col == other.col;
    }

    bool operator!=(const LevelIndex& other) const {
        return !(*this == other);
    }
}; 

namespace std {
    template<> struct hash<LevelIndex> {
        size_t operator()(LevelIndex const& lvlIndex) const {
            return (((lvlIndex.row) ^ ((lvlIndex.col) << 1)) >> 1);
        }
    };
}

class LevelMap {
public:
    LevelMap(const char* path);

    void loadModel(Model& model);
private:
    void loadLevelData(const char* filePath);

    prt::vector<prt::vector<uint32_t> > _levelData;
    prt::vector<std::string> _texturePaths;

    void findIsland(LevelIndex lvlIndex, prt::vector<LevelIndex>& island);
    void find4Neighbours(LevelIndex lvlIndex, prt::vector<LevelIndex>& neighbours);
};

#endif