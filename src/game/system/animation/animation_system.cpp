#include "animation_system.h"

AnimationID AnimationSystem::addAnimation(EntityID entityID) { 
    AnimationID id = m_animationBlends.size();
    m_animationBlends.push_back({}); 

    m_entityToAnimation.insert(entityID, id);

    return id;
};
