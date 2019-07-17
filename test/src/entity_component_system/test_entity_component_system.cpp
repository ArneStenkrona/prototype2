#include "test/src/prt_test.h"
#include "src/entity_component_system/entity/entity_manager.h"
#include <catch2/catch.hpp>

class IntComponent : public Component {
public:
    IntComponent() : x(0) {};
    uint16_t x;
};

TEST_CASE( "Test add component", "[pool_allocator]" ) {
    size_t memory = 64 * 1024;
    EntityManager entityManager = EntityManager(
                                malloc(memory), 
                                memory);

    EntityID id = entityManager.createEntity();

    entityManager.addComponent<IntComponent>(id);
}