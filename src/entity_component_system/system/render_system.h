#ifndef RENDER_SYSTEM_H
#define RENDER_SYSTEM_H

#include "src/entity_component_system/component/component_manager.h"
#include "src/entity_component_system/component/component.h"

#include "src/config/prototype2Config.h"

#include "src/container/array.h"
#include "src/container/vector.h"

//class RenderSystem {
//public:
//    RenderSystem();
//    
//    void update(prt::vector<Model>& models, 
//                prt::array<EntityID, TOTAL_ENTITIES> modelIndexToEntityID,
//                prt::vector<Transform> transforms,
//                prt::array<EntityID, TOTAL_ENTITIES> entityIDToTransformIndex);
//
//    void render();
//
//private:
//    struct RenderJob { 
//        Model model;
//        Transform transform;
//    };
//    prt::vector<RenderJob> _renderQueue;
//};

#endif