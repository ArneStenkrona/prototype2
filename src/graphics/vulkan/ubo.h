#ifndef UBO_H
#define UBO_H

#include "src/config/prototype2Config.h"
#include "src/graphics/lighting/light.h"

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

struct ModelUBO {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 invTransposeModel[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 view;
    alignas(16) glm::mat4 proj;
    alignas(16) glm::vec3 viewPosition;
    alignas(4)  float t = 0;
    // unsigned char pad12[12];
};

struct LightUBO {
    alignas(4)  float ambientLight;
    alignas(4)  int32_t noPointLights;
    alignas(16) DirLight sun;
    alignas(16) glm::vec4 splitDepths[(NUMBER_SHADOWMAP_CASCADES + 4)/ 4];
    alignas(16) glm::mat4 cascadeSpace[NUMBER_SHADOWMAP_CASCADES];
    alignas(16) PointLight pointLights[NUMBER_SUPPORTED_POINTLIGHTS];
};

struct BoneUBO {
    glm::mat4 bones[NUMBER_MAX_BONES];
};

struct StandardUBO {
    ModelUBO model;
    LightUBO lighting;
};

struct StandardPushConstants {
    alignas(4)  int32_t modelMatrixIdx;
	alignas(4)  int32_t albedoIndex;
	alignas(4)  int32_t normalIndex;
	alignas(4)  int32_t specularIndex;
    alignas(16) glm::vec3 baseColor;
    alignas(4)  float baseSpecularity;
};

struct SkyboxUBO {
		alignas(16) glm::mat4 projection;
		alignas(16) glm::mat4 model;
		//alignas(4) float lodBias = 0.0f;
};

struct ShadowMapUBO  {
    alignas(16) glm::mat4 model[NUMBER_SUPPORTED_MODEL_MATRICES];
    alignas(16) glm::mat4 depthVP[NUMBER_SHADOWMAP_CASCADES];
};

#endif
