#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include "src/game/level/level_map.h"

#include "src/graphics/geometry/parametric_shapes.h"

#include "src/graphics/geometry/model.h"

#include <dirent.h>

class ModelManager {
public:
    ModelManager(const char* directory);

    inline void insertQuads(prt::vector<parametric_shapes::Quad>& quads){
        insertParametric(_quads, quads, "QUAD");    
    }
    inline void insertCuboids(prt::vector<parametric_shapes::Cuboid>& cuboids){
        insertParametric(_cuboids, cuboids, "CUBOID");    
    }
    inline void insertSpheres(prt::vector<parametric_shapes::Sphere>& spheres){
        insertParametric(_spheres, spheres, "SPHERE");    
    }
    inline void insertCylinders(prt::vector<parametric_shapes::Cylinder>& cylinders) {
        insertParametric(_cylinders, cylinders, "CYLINDER");    
    }
    inline void insertCapsules(prt::vector<parametric_shapes::Capsule>& capsules) {
        insertParametric(_capsules, capsules, "CAPSULE");    
    }

    void loadModels(prt::vector<Model>& models);

    void getPaths(prt::vector<std::string>& modelPaths, prt::vector<prt::vector<std::string> >& texturePaths);
    uint32_t getModelID(std::string& name);
    uint32_t getModelID(const char* name);

    static constexpr uint32_t UNDEFINED_MODEL = -1;
    static constexpr uint32_t DEFAULT_MODEL = 0;
    
private:
    prt::hash_map<std::string, std::string> _modelPaths;
    prt::hash_map<std::string, prt::vector<std::string> > _texturePaths;
    prt::hash_map<std::string, uint32_t> _modelIDs;

    // Parametrics
    prt::vector<parametric_shapes::Quad> _quads;
    prt::vector<parametric_shapes::Cuboid> _cuboids;
    prt::vector<parametric_shapes::Sphere> _spheres;
    prt::vector<parametric_shapes::Cylinder> _cylinders;
    prt::vector<parametric_shapes::Capsule> _capsules;

    //LevelMap _levelMap;

    std::string _directory;
    uint16_t nextID = 0;
    
    void loadPersistentPaths(const char* directory);
    void loadOBJ(const char* modelPath, Model& model);
    void loadParametric(const char* modelPath, Model& model);

    template<class T>
    void insertParametric(prt::vector<T>& dest, prt::vector<T>& source, std::string baseName) {
        std::string nonPersistentModelAssetPath = "N:" + std::string("/model/");
        std::string modelAssetPath = std::string("P:") + _directory;
        uint32_t currInd = dest.size();
        dest.resize(dest.size() + source.size());
        for (uint32_t i = 0; i < source.size(); i++) {
            dest[currInd] = source[i];

            _modelPaths.insert(baseName + std::to_string(currInd), nonPersistentModelAssetPath + baseName + "/" + std::to_string(currInd));
            _texturePaths.insert(baseName + std::to_string(currInd), { modelAssetPath + "DEFAULT/diffuse.png" });
            _modelIDs.insert(baseName + std::to_string(currInd), nextID++);
            currInd++;
        }
    }
};

#endif