#include "character_system.h"

#include "src/util/math_util.h"

#include "glm/gtx/euler_angles.hpp"

#include "glm/gtx/string_cast.hpp"

#include "src/game/scene/scene.h"

#include <cstdlib>

CharacterSystem::CharacterSystem(Scene * scene, 
                                 PhysicsSystem & physicsSystem)
    : m_scene{scene},
      m_physicsSystem{physicsSystem},
      m_playerController{scene->getInput(), scene->getCamera()} {
}

void CharacterSystem::updateCharacters(float deltaTime) {
    // player input
    m_playerController.updateInput(m_characters[PLAYER_ID].input);

    // JUST FOR FUN, WILL REMOVE LATER
    for (size_t i = 1; i < m_characters.size(); ++i) {
        Character & character = m_characters[i];
        glm::vec3 dir = m_scene->getTransform(m_characters[0].id).position - m_scene->getTransform(character.id).position;
        character.input.move = glm::vec2(dir.x,dir.z);
        if (glm::length2(character.input.move) > 0.0f) character.input.move = glm::normalize(character.input.move);
    }

    for (Character & character : m_characters) {
        updateCharacter(character, deltaTime);
    }
}

CharacterID CharacterSystem::addCharacter(EntityID entityID, ColliderTag tag) { 
    assert(m_characters.size() < std::numeric_limits<CharacterID>::max() && "Character amount exceeded!");
    m_characters.push_back({});
    Character & character = m_characters.back();

    character.id = entityID;
    character.physics.colliderTag = tag;

    return m_characters.size() - 1; 
}

void CharacterSystem::addEquipment(CharacterID characterID, int boneIndex, EntityID equipment, Transform offset) {
    m_characters[characterID].attributeInfo.equipment.push_back({});
    m_characters[characterID].attributeInfo.equipment.back().entity = equipment;
    m_characters[characterID].attributeInfo.equipment.back().offset = offset;
    m_characters[characterID].attributeInfo.equipment.back().boneIndex = boneIndex;
}
           
void CharacterSystem::updatePhysics(float deltaTime) {
    prt::vector<Transform> transforms;
    transforms.resize(m_characters.size());
 
    {
        int i = 0;
        for (Character & character : m_characters) {
            transforms[i] = m_scene->getTransform(character.id);
            ++i;
        }
    }

    m_physicsSystem.updateCharacters(deltaTime,
                                     *m_scene,
                                     transforms.data());
    {
        int i = 0;
        for (Character & character : m_characters) {
            m_scene->getTransform(character.id) = transforms[i];
            character.attributeInfo.updateEquipment(character.id, *m_scene);
            ++i;
        }
    }
}

void CharacterSystem::updateCharacter(Character & character, float deltaTime) {
    auto & attributeInfo = character.attributeInfo;
    auto & physics = character.physics;
    auto & animation = m_scene->getAnimationSystem().getAnimationComponent(character.id);

    updateCharacterInput(character, deltaTime);

    attributeInfo.updateState(deltaTime, animation, physics);

    switch (attributeInfo.stateInfo.getState()) {
        case CHARACTER_STATE_JUMPING: {
            if (attributeInfo.stateInfo.getStateChange()) {
                physics.velocity.y = 0.35f;
            }
            
            break;
        }
        default: {}
    }
    
    setStateTransitions(character);
}

void CharacterSystem::updateCharacterInput(Character & character, float /*deltaTime*/) {
    auto const & input = character.input;
    auto & transform = m_scene->getTransform(character.id);
    auto & physics = character.physics;
    auto & stateInfo = character.attributeInfo.stateInfo;
    
    glm::vec3 targetMovement{0.0f};

    // check if character performed any movement input
    if (glm::length2(input.move) > 0.0f) {
        glm::vec3 const origin{0.0f, 0.0f, 0.0f}; 
        glm::vec3 const up{0.0f, 1.0f, 0.0f}; 
        glm::vec3 const altUp{0.0f, 0.0f, 1.0f}; 
        // compute movement plane
        glm::vec3 const moveNormal = physics.isGrounded ? physics.groundNormal : up;
        
        glm::quat groundRot = glm::rotation(up, moveNormal);

        if (stateInfo.getCanTurn()) {
            glm::vec3 const newDir = glm::rotate(groundRot, glm::vec3{input.move.x,  0.0f, input.move.y});
            physics.forward = glm::dot(physics.forward, newDir) > -0.9f ?
                                       glm::normalize(glm::mix(physics.forward, newDir, 0.5f)) :
                                       newDir;
            // rotate character
            transform.rotation = math_util::safeQuatLookAt(origin, glm::vec3{input.move.x,  0.0f, input.move.y}, up, altUp);
        }


        targetMovement = physics.forward * stateInfo.getMovementSpeed();
    }

    physics.movementVector = targetMovement;
}

void CharacterSystem::setStateTransitions(Character & character) {
    auto const & input = character.input;
    auto & stateInfo = character.attributeInfo.stateInfo;
    auto & physics = character.physics;
    auto & animation = m_scene->getAnimationSystem().getAnimationComponent(character.id);

    stateInfo.resetStateChange();

    switch (stateInfo.getState()) {
        case CHARACTER_STATE_IDLE: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            } else if (glm::length2(input.move) > 0.0f) {
                stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
            } else if (input.attack) {
                stateInfo.transitionState(CHARACTER_STATE_SLASH1, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_WALKING: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
               stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            } else if (input.attack) {
                stateInfo.transitionState(CHARACTER_STATE_SLASH1, 0.0f);
            } else if (glm::length2(input.move) > 0.0f) {
                if (input.run) stateInfo.transitionState(CHARACTER_STATE_RUNNING, 0.1f);
            } else {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
            } 
            break;
        }
        case CHARACTER_STATE_RUNNING: {
            if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            } else if (stateInfo.getGroundedTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.0f);
            } else if (input.attack) {
                stateInfo.transitionState(CHARACTER_STATE_SLASH1, 0.0f);
            } else if (glm::length2(input.move) > 0.0f) {
                if (!input.run) stateInfo.transitionState(CHARACTER_STATE_WALKING, 0.1f);
            } else {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
            }
            break;
        }
        case CHARACTER_STATE_JUMPING: {
            if (physics.isGrounded && stateInfo.getStateTimer() > 0.1f) {
                stateInfo.transitionState(CHARACTER_STATE_LANDING_MILDLY, 0.05f);
            } else if (animation.clipA.isCompleted()) {
                stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.3f);
            } else if (input.attack && stateInfo.getStateTimer() > 0.1f) {
                stateInfo.transitionState(CHARACTER_STATE_MIDAIR_SLASH1, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_FALLING: {
            if (physics.isGrounded) {
                if (glm::length2(input.move) > 0.0f && stateInfo.getStateTimer() > 0.2f) {
                    stateInfo.transitionState(CHARACTER_STATE_ROLLING, 0.0f);
                } else {
                    if (stateInfo.getStateTimer() < 0.4f) {
                        stateInfo.transitionState(CHARACTER_STATE_LANDING_MILDLY, 0.05f);
                    } else {
                        stateInfo.transitionState(CHARACTER_STATE_LANDING, 0.0f);
                    }
                }
            } else if (input.attack) {
                stateInfo.transitionState(CHARACTER_STATE_MIDAIR_SLASH1, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_LANDING:
        case CHARACTER_STATE_LANDING_MILDLY: {
            if (glm::length2(input.move) > 0.0f) {
                stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
            } else if (animation.clipA.isCompleted()) {
                stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.3f);
            } else if (stateInfo.getGroundedTimer() <= 0.15f && input.jump) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_ROLLING: {
            if (input.jump && stateInfo.getStateTimer() > 0.1f) {
                stateInfo.transitionState(CHARACTER_STATE_JUMPING, 0.1f);
            } else if (animation.clipA.isCompleted()) {
                if (glm::length2(input.move) > 0.0f) {
                    stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
                } else {
                    stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.3f);
                }
            }
            break;
        }
        case CHARACTER_STATE_SLASH1: {
            if (animation.clipA.isCompleted()) {
                if (glm::length2(input.move) > 0.0f) {
                    stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
                } else {
                    stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
                }
            } else if (input.attack && stateInfo.getStateTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_SLASH2, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_SLASH2: {
            if (animation.clipA.isCompleted()) {
                if (glm::length2(input.move) > 0.0f) {
                    stateInfo.transitionState(input.run ? CHARACTER_STATE_RUNNING : CHARACTER_STATE_WALKING, 0.1f);
                } else {
                    stateInfo.transitionState(CHARACTER_STATE_IDLE, 0.1f);
                }
            } else if (input.attack && stateInfo.getStateTimer() > 0.15f) {
                stateInfo.transitionState(CHARACTER_STATE_SLASH1, 0.0f);
            }
            break;
        }
        case CHARACTER_STATE_MIDAIR_SLASH1: {
            if (animation.clipA.isCompleted()) {
               if (physics.isGrounded) {
                    stateInfo.transitionState(CHARACTER_STATE_ROLLING, 0.1f);
                } else {
                    stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.1f);
                }
            }  else if (stateInfo.getStateTimer() > 0.15f) {
                if (physics.isGrounded) {
                    stateInfo.transitionState(CHARACTER_STATE_ROLLING, 0.1f);
                } else if (input.attack) {
                    stateInfo.transitionState(CHARACTER_STATE_MIDAIR_SLASH2, 0.0f);
                }
            }
            break;
        }
        case CHARACTER_STATE_MIDAIR_SLASH2: {
            if (animation.clipA.isCompleted()) {
                if (physics.isGrounded) {
                    stateInfo.transitionState(CHARACTER_STATE_ROLLING, 0.1f);
                } else {
                    stateInfo.transitionState(CHARACTER_STATE_FALLING, 0.1f);
                }
            } else if (stateInfo.getStateTimer() > 0.15f) {
                if (physics.isGrounded) {
                    stateInfo.transitionState(CHARACTER_STATE_ROLLING, 0.1f);
                } else if (input.attack) {
                    stateInfo.transitionState(CHARACTER_STATE_MIDAIR_SLASH1, 0.0f);
                }
            }
            break;
        }

        default: {}
    }
}
