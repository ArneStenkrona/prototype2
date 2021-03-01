#ifndef LIGHTING_SYSTEM_H
#define LIGHTING_SYSTEM_H

#include "src/container/vector.h"
#include "src/graphics/lighting/light.h"
#include "src/game/scene/id.h"
#include "src/graphics/camera/camera.h"
#include "src/game/component/component.h"

#include <cstdint>

enum LightType : uint16_t {
    LIGHT_TYPE_NONE,
    LIGHT_TYPE_POINT,
    // LIGHT_TYPE_DIRECTIONAL,
    TOTAL_NUM_LIGHT_TYPES
};

struct PointLight {
    glm::vec3 color;
    float constant;
    float linear;
    float quadratic;  
    EntityID entityID;
};

typedef uint16_t LightIndex;

struct LightTag {
    LightIndex index;
    LightType type = LightType::LIGHT_TYPE_NONE;
    friend bool operator== (LightTag const & c1, LightTag const & c2) {
        return (c1.index == c2.index &&
                c1.type == c2.type);
    }
    friend bool operator!= (LightTag const & c1, LightTag const & c2)  {
        return !(c1 == c2);
    }
};

class LightingSystem {
public:
    LightTag addPointLight(PointLight const & pointLight);

    PointLight & getPointLight(LightTag const & tag);

    prt::vector<UBOPointLight> getNearestPointLights(Camera const & camera, Transform const * transforms);
    
private:
    prt::vector<PointLight> m_pointLights;
};

#endif
