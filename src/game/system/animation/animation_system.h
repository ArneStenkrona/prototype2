#ifndef PRT_ANIMATION_SYSTEM_H
#define PRT_ANIMATION_SYSTEM_H

#include "src/game/scene/entity.h"

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include <cstdint>

/* Animation blending */
struct BlendedAnimation {
    uint32_t clipA;
    uint32_t clipB;
    float blendFactor = 0.0f;
    float time = 0.0f;
    bool paused = false;
};

class AnimationSystem {
public:
    AnimationID addAnimation(EntityID entityID);

    inline BlendedAnimation & getAnimationBlend(EntityID entityID) { return  m_animationBlends[m_entityToAnimation[entityID]]; }

    void getAnimationBlends(BlendedAnimation * & blends, size_t & n) { blends = m_animationBlends.data();
                                                                       n = m_animationBlends.size(); }

private:
    prt::vector<BlendedAnimation> m_animationBlends;
    prt::hash_map<EntityID, AnimationID> m_entityToAnimation;

};

#endif
