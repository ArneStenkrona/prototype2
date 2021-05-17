#ifndef PRT_ANIMATION_SYSTEM_H
#define PRT_ANIMATION_SYSTEM_H

#include "src/game/scene/entity.h"

#include "src/container/hash_map.h"
#include "src/container/vector.h"

#include <cstdint>

struct RenderData;
class ModelManager;
class Scene;

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
    AnimationSystem(ModelManager & modelManager, Scene & scene);

    AnimationID addAnimation(EntityID entityID);

    void updateAnimation(ModelID const * modelIDs, size_t n);
    prt::vector<glm::mat4> const & getBoneTransforms();

    glm::mat4 getCachedTransformation(EntityID entityID, int boneIndex) const;
    glm::mat4 getCachedTransformation(EntityID entityID, char const * boneName) const;

    inline BlendedAnimation & getAnimationBlend(EntityID entityID) { return  m_animationBlends[m_entityToAnimation[entityID]]; }

    void getAnimationBlends(BlendedAnimation * & blends, size_t & n) { blends = m_animationBlends.data();
                                                                       n = m_animationBlends.size(); }

private:
    prt::vector<BlendedAnimation> m_animationBlends;
    prt::hash_map<EntityID, AnimationID> m_entityToAnimation;
    prt::vector<glm::mat4> m_boneTransforms;
    prt::vector<uint32_t> m_boneOffsets;

    ModelManager & m_modelManager;
    Scene        & m_scene;
};

#endif
