#include "animation_system.h"

#include "src/game/scene/scene.h"
#include "src/system/assets/model_manager.h"

AnimationSystem::AnimationSystem(ModelManager & modelManager, Scene & scene)
 : m_modelManager{modelManager}, m_scene{scene} {
    m_boneOffsets.push_back(0);
}

AnimationID AnimationSystem::addAnimation(EntityID entityID) { 
    AnimationID id = m_animationComponents.size();
    m_animationComponents.push_back({}); 

    m_entityToAnimation.insert(entityID, id);
    m_boneOffsets.push_back(m_modelManager.getModel(m_scene.getModelID(entityID)).getNumBones());
    m_boneTransforms.resize(m_modelManager.getModel(m_scene.getModelID(entityID)).getNumBones());

    return id;
};
void AnimationSystem::updateAnimation(float deltaTime, ModelID const * modelIDs, size_t n) {
    for (AnimationComponent & component : m_animationComponents) {
        component.clipA.update(deltaTime);
        component.clipB.update(deltaTime);
    }

    m_modelManager.sampleAnimation(modelIDs,
                                   m_animationComponents.data(),
                                   m_boneTransforms,
                                   n);
}

prt::vector<glm::mat4> const &  AnimationSystem::getBoneTransforms() {
    return m_boneTransforms;
}
    
glm::mat4 AnimationSystem::getCachedTransformation(EntityID entityID, char const * boneName) const {
    int boneIndex = m_scene.getModel(entityID).getBoneIndex(boneName);
    if (boneIndex == -1) {
        assert(false);
    }

    return getCachedTransformation(entityID, boneIndex);
}

glm::mat4 AnimationSystem::getCachedTransformation(EntityID entityID, int boneIndex) const {
    AnimationID id = m_entityToAnimation[entityID];
    return m_boneTransforms[m_boneOffsets[id] + boneIndex];
}
