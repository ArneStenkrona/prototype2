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
                    PhysicsSystem & physicsSystem);

    CharacterID addCharacter(EntityID entityID, ColliderTag tag);

    void addEquipment(CharacterID characterID, int boneIndex, EntityID equipment, Transform offset);
             
    void updateCharacters(float deltaTime);

    void updatePhysics(float deltaTime);

    Character & getCharacter(CharacterID id) { return m_characters[id]; }
    size_t getNumberOfCharacters() const { return m_characters.size(); }
    // prt::vector<Character> & getCharacters() { return m_characters; }

    EntityID getPlayer() const { return m_characters[PLAYER_ID].id; }
    CharacterID getPlayerCharacterID() const { return PLAYER_ID; }

private:
    // Characters<10> m_characters;
    prt::vector<Character> m_characters;

    static constexpr CharacterID PLAYER_ID = 0;

    Scene * m_scene;
    PhysicsSystem & m_physicsSystem;

    PlayerController m_playerController;

    void updateCharacter(Character & character, float deltaTime);
    void updateEquipment(Character & character);
    void updateCharacterInput(Character & character, float deltaTime);

    void setStateTransitions(Character & character);

    friend class SceneSerialization;
};

#endif
