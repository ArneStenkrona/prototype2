#include "animation_system.h"

#include "src/game/scene/scene.h"
#include "src/system/assets/model_manager.h"

AnimationSystem::AnimationSystem(ModelManager & modelManager, Scene & scene)
 : m_modelManager{modelManager}, m_scene{scene} {
    m_boneOffsets.push_back(0);
}

AnimationID AnimationSystem::addAnimation(EntityID entityID) { 
    AnimationID id = m_animationBlends.size();
    m_animationBlends.push_back({}); 

    m_entityToAnimation.insert(entityID, id);
    m_boneOffsets.push_back(m_modelManager.getModel(m_scene.getModelID(entityID)).getNumBones());
    m_boneTransforms.resize(m_modelManager.getModel(m_scene.getModelID(entityID)).getNumBones());


    return id;
};
void AnimationSystem::updateAnimation(ModelID const * modelIDs, size_t n) {
    BlendedAnimation * blends;
    size_t nBlends;

    getAnimationBlends(blends, nBlends);

    m_modelManager.getSampledBlendedAnimation(modelIDs,
                                              blends,
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
