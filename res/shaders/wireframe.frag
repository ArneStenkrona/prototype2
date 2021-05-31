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

layout(push_constant) uniform MATERIAL {
    layout(offset = 0) int   modelMatrixIndex;
} material;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec3 fragNormal;
} fs_in;

layout(location = 0) out vec4 outColor;
layout(location = 1) out int entityID;

void main() {
    outColor = vec4(0.0,1.0,0.0,1.0);
    entityID = -1;
}