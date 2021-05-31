#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    /* Model */
    mat4 model[200];
    mat4 invTransposeModel[200];
    mat4 viewProjection;
    mat4 view;
    // mat4 proj;
    vec3 viewPos;
    float t;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelMatrixIndex;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;

layout(location = 0) out VS_OUT {
    vec3 fragPos;
    vec3 fragNormal;
} vs_out;

void main() {
    vec4 worldPos = ubo.model[pc.modelMatrixIndex] * vec4(inPosition, 1.0);
    worldPos = worldPos / worldPos.w;
    vs_out.fragPos = worldPos.xyz;

    vec3 n = normalize(vec3(ubo.invTransposeModel[pc.modelMatrixIndex] * vec4(inNormal, 0.0)));
    
    vs_out.fragNormal = n;
    
    gl_Position = ubo.viewProjection * worldPos;
}
