#include "render_system.h"

RenderSystem::RenderSystem()
: _renderQueue() {}

void RenderSystem::update(prt::vector<Model>& models, 
                          prt::array<EntityID, TOTAL_ENTITIES> modelIndexToID,
                          prt::vector<Transform> transforms,
                          prt::array<EntityID, TOTAL_ENTITIES> entityIDToTransformIndex) {
    for (size_t i = 0; i < models.size(); i++) {
        Model& model = models[i];
        EntityID tIndex = entityIDToTransformIndex[modelIndexToID[i]];
        Transform transform = transforms[tIndex];
        // addToRenderQueue(model, transform);
        _renderQueue.push_back( {model, transform} );
    }
}