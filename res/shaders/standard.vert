#version 450
#extension GL_ARB_separate_shader_objects : enable

struct DirLight {
    vec3 direction;
    vec3 color;
};

struct PointLight {
    vec3 pos;
    float a; // quadtratic term
    vec3 color;
    float b; // linear term
    float c; // constant term
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    /* Model */
    mat4 model[100];
    mat4 invTransposeModel[100];
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float t;
    /* Lights */
    float ambientLight;
    int noPointLights;
    DirLight sun;
    vec4 splitDepths[(4 + 4) / 4];
    mat4 cascadeSpace[4];
    PointLight pointLights[4];
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelMatrixIdx;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;

layout(location = 0) out VS_OUT {
    vec3 fragPos;
    //vec3 normal;
    vec2 fragTexCoord;
    //float t;
    //vec3 viewDir;
    // vec3 viewPos;
    vec3 shadowPos;
    //mat3 tbn;
    vec3 tangentSunDir;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
    // vec4 sunShadowCoord;
} vs_out;

void main() {
    vs_out.fragPos = vec3(ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0));
    vec3 t = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inTangent);
    vec3 b = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inBinormal);
    vec3 n = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inNormal);

    mat3 tbn = transpose(mat3(t,b,n));
    
    vs_out.fragTexCoord = inTexCoord;

    vs_out.shadowPos = (ubo.view * vec4(vs_out.fragPos + inNormal, 1.0)).xyz;

    vs_out.tangentSunDir = tbn * ubo.sun.direction;
    vs_out.tangentViewPos = tbn * ubo.viewPos;
    vs_out.tangentFragPos = tbn * vs_out.fragPos;

    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0);
}
