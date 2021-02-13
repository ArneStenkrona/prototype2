#include "lighting_system.h"

#include "src/config/prototype2Config.h"

#include <glm/glm.hpp>

LightTag LightingSystem::addPointLight(PointLight const & pointLight) {
    LightTag tag;
    tag.index = m_pointLights.size();
    tag.type = LightType::LIGHT_TYPE_POINT;

    m_pointLights.push_back(pointLight);

    return tag;
}

PointLight & LightingSystem::getPointLight(LightTag const & tag) {
    assert(tag.type == LightType::LIGHT_TYPE_POINT);

    return m_pointLights[tag.index];
}

struct IndexedDistance {
    unsigned int index;
    float distance;
};
int compLightsToCamera(const void * elem1, const void * elem2) {
    IndexedDistance f = *((IndexedDistance*)elem1);
    IndexedDistance s = *((IndexedDistance*)elem2);
    if (f.distance > s.distance) return  1;
    if (f.distance < s.distance) return -1;
    return 0;
}

prt::vector<UBOPointLight> LightingSystem::getNearestPointLights(Camera const & camera, Transform const * transforms) {
    prt::vector<UBOPointLight> ret;
    
    prt::vector<IndexedDistance> distances;
    distances.resize(m_pointLights.size());

    for (size_t i = 0; i < m_pointLights.size(); ++i) {
        EntityID eID = m_pointLights[i].entityID;
        distances[i].index = i;
        distances[i].distance = glm::distance2(transforms[eID].position, camera.getPosition());
    }
    // sort lights by distance to camera
    qsort(distances.data(), distances.size(), sizeof(distances[0]), compLightsToCamera);

    size_t size = glm::min(size_t(NUMBER_SUPPORTED_POINTLIGHTS), m_pointLights.size());
    ret.resize(size);
    for (size_t i = 0; i < size; ++i) {
        ret[i].color = m_pointLights[distances[i].index].color;
        ret[i].c = m_pointLights[distances[i].index].constant;
        ret[i].b = m_pointLights[distances[i].index].linear;
        ret[i].a = m_pointLights[distances[i].index].quadratic;

        EntityID eID = m_pointLights[distances[i].index].entityID;
        ret[i].pos = transforms[eID].position;
    }
    return ret;
}
