#ifndef PRT_ANIMATION_SYSTEM_H
#define PRT_ANIMATION_SYSTEM_H

#include "src/game/scene/entity.h"

#include "src/container/hash_map.h"
#include "src/container/vector.h"


#include "animation_clip.h"

#include <cstdint>

struct RenderData;
class ModelManager;
class Scene;

struct AnimationComponent {
    AnimationClip clipA;
    AnimationClip clipB;
    float blendFactor;
};

class AnimationSystem {
public:
    AnimationSystem(ModelManager & modelManager, Scene & scene);

    AnimationID addAnimation(EntityID entityID);

    void updateAnimation(float deltaTime, ModelID const * modelIDs, size_t n);
    prt::vector<glm::mat4> const & getBoneTransforms();

    glm::mat4 getCachedTransformation(EntityID entityID, int boneIndex) const;
    glm::mat4 getCachedTransformation(EntityID entityID, char const * boneName) const;

    inline AnimationComponent & getAnimationComponent(EntityID entityID) { return  m_animationComponents[m_entityToAnimation[entityID]]; }

private:
    prt::vector<AnimationComponent> m_animationComponents;
    prt::hash_map<EntityID, AnimationID> m_entityToAnimation;
    prt::vector<glm::mat4> m_boneTransforms;
    prt::vector<uint32_t> m_boneOffsets;

    ModelManager & m_modelManager;
    Scene        & m_scene;
};

#endif
