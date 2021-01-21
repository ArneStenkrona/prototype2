#ifndef PRT_ENTITY_H
#define PRT_ENTITY_H

#include "src/game/component/component.h"

#include "src/container/vector.h"

#include <cstdint>

typedef int32_t CharacterID;
typedef int32_t EntityID;
typedef int32_t AnimationID;
typedef int32_t ModelID;

// template<size_t N>
struct Entities {
public:
    static constexpr size_t N = 100;
    static constexpr size_t SIZE_STR = 64;

    enum { maxSize = N };
    char        names[N][SIZE_STR];
    Transform   transforms[N];
    ModelID     modelIDs[N];
    AnimationID animationIDs[N];
    CharacterID characterIDs[N];
    ColliderTag colliderTags[N];

    EntityID addEntity() { ++nEntities; return nEntities - 1; }
    EntityID size() const { return nEntities; }
private:
    EntityID nEntities = 0;
};

#endif
