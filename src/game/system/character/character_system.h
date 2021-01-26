#ifndef CHARACTER_H
#define CHARACTER_H

#include "src/game/scene/scene_serialization.h"
#include "player.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/system/physics/physics_system.h"
#include "src/game/component/component.h"
#include "src/game/scene/entity.h"
#include "src/container/vector.h"

#include <cstdint>

class Scene;

enum CharacterType {
    CHARACTER_TYPE_NONE,
    CHARACTER_TYPE_PLAYER,
    CHARACTER_TYPE_NPC
};

class CharacterSystem {
public:
    CharacterSystem(Scene * scene, 
                    PhysicsSystem & physicsSystem,
                    AnimationSystem & animationSystem);

    CharacterID addCharacter(EntityID entityID, ColliderTag tag, CharacterAnimationClips clips);
             
    void updateCharacters(float deltaTime);

    void updatePhysics(float deltaTime);

    EntityID getPlayer() const { return m_characters.entityIDs[PLAYER_ID]; }

    // TODO: better way to set and denote
    // character types
    CharacterType getType(CharacterID id) const { if (id == -1) return CHARACTER_TYPE_NONE; 
                                                  return id == PLAYER_ID ? CHARACTER_TYPE_PLAYER : CHARACTER_TYPE_NPC; }

private:
    template<size_t N>
    struct Characters {
        enum { maxSize = N };
        CharacterID size = 0;

        EntityID entityIDs[N];
        CharacterPhysics physics[N];
        CharacterInput input[N];
        CharacterAnimationClips animationClips[N];
    };
    Characters<10> m_characters;
    static constexpr size_t PLAYER_ID = 0;

    Scene * m_scene;
    PhysicsSystem & m_physicsSystem;
    AnimationSystem & m_animationSystem;

    PlayerController m_playerController;

    void updateCharacter(CharacterID characterID, float deltaTime);

    friend class SceneSerialization;
};

#endif
