#ifndef MODEL_MANAGER_H
#define MODEL_MANAGER_H

#include "src/container/hash_map.h"
#include "src/container/vector.h"

typedef char ModelName[128];



namespace std {
    template<> struct hash<ModelName> {
        size_t operator()(ModelName const& modelName) const {
            size_t result = 0;
            const size_t prime = 31;
            for (size_t i = 0; i < 128; ++i) {
                result = modelName[i] + (result * prime);
            }
            return result;
        }
    };
}

class ModelManager {
public:
    ModelManager(const char* directory);
    
private:
    prt::hash_map<std::string, std::string> _modelPaths;

};

#endif