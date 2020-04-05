#version 450
#extension GL_ARB_separate_shader_objects : enable

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
    PointLight pointLights[4];//[@NUMBER_SUPPORTED_POINTLIGHTS@];
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelMatrixIdx;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBitangent;

layout(location = 0) out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
    float t;
    vec3 viewDir;
    mat3 tbn;
} vs_out;

void main() {
    vs_out.fragPos = vec3(ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0));
    vec3 t = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inTangent);
    vec3 b = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inBitangent);
    vec3 n = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inNormal);

    vs_out.normal = n;
    vs_out.tbn = mat3(t,b,n);
    
    vs_out.fragTexCoord = inTexCoord;
    vs_out.t = ubo.t;
    vs_out.viewDir = ubo.viewPos - inPosition;

    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0);
}
