#ifndef PRT_CHARACTER_SYSTEM_H
#define PRT_CHARACTER_SYSTEM_H

#include "src/game/scene/scene_serialization.h"
#include "src/game/system/character/player.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/system/physics/physics_system.h"
#include "src/game/system/character/character.h"
#include "src/game/component/component.h"
#include "src/game/scene/entity.h"
#include "src/container/vector.h"

#include <cstdint>

class Scene;

class CharacterSystem {
public:
    CharacterSystem(Scene * scene, 
                    PhysicsSystem & physicsSystem,
                    AnimationSystem & animationSystem);

    CharacterID addCharacter(EntityID entityID, ColliderTag tag);
             
    void updateCharacters(float deltaTime);

    void updatePhysics(float deltaTime);

    EntityID getPlayer() const { return m_characters.entityIDs[PLAYER_ID]; }
    CharacterID getPlayerCharacterID() const { return PLAYER_ID; }

    CharacterPhysics const & getCharacterPhysics(CharacterID id) const { return m_characters.physics[id]; }
    CharacterInput const & getCharacterInput(CharacterID id) const { return m_characters.input[id]; }

    // TODO: better way to set and denote
    // character types
    CharacterType getType(CharacterID id) const { return m_characters.attributeInfos[id].type; }

private:
    Characters<10> m_characters;
    static constexpr CharacterID PLAYER_ID = 0;

    Scene * m_scene;
    PhysicsSystem & m_physicsSystem;
    AnimationSystem & m_animationSystem;

    PlayerController m_playerController;

    void updateCharacter(CharacterID characterID, float deltaTime);
    void updateEquipment(CharacterID characterID);
    void updateCharacterInput(CharacterID characterID, float deltaTime);

    void setStateTransitions(CharacterID characterID);

    friend class SceneSerialization;
};

#endif
