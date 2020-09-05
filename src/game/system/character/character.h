#ifndef CHARACTER_H
#define CHARACTER_H

#include <cstdint>

#include "player.h"
#include "src/system/assets/asset_manager.h"
#include "src/game/system/physics/physics_system.h"
#include "src/game/component/component.h"
#include "src/container/vector.h"

class CharacterSystem {
public:
    CharacterSystem(Input & input, 
                    Camera & camera, 
                    PhysicsSystem & physicsSystem, 
                    AssetManager & assetManager);

    void initPlayer();
    void addCharacter(uint32_t modelID);
    void updateCharacters(float deltaTime);

    void updatePhysics();
    void updateCamera();

    void sampleAnimation(prt::vector<glm::mat4> & bones);

    uint32_t const * getModelIDs(size_t & nModelIDs) const { nModelIDs = m_characters.size; return m_characters.modelIDs; }
    void getTransformMatrices(prt::vector<glm::mat4>& transformMatrices) const;

private:
    template<size_t N>
    struct Characters {
        enum { maxSize = N };
        size_t size = 0;

        uint32_t modelIDs[N];

        Transform transforms[N];
        CharacterPhysics physics[N];

        CharacterInput input[N];
        
        BlendedAnimation animation[N];
        CharacterAnimationClips animationClips[N];
    };
    Characters<10> m_characters;
    static constexpr size_t PLAYER_ID = 0;

    Camera & m_camera;
    PlayerController m_playerController;
    PhysicsSystem & m_physicsSystem;

    AssetManager & m_assetManager;

    void updateCharacter(size_t index, float deltaTime);

};

#endif
