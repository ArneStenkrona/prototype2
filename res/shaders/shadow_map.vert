#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    /* Model */
    mat4 model[200];
    mat4 depthVP[4];
} ubo;

layout(push_constant) uniform PER_OBJECT {
	layout(offset = 0) int modelMatrixIdx;
    layout(offset = 4) int cascadeIndex; 
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;

void main() {
    gl_Position = ubo.depthVP[pc.cascadeIndex] * ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0);
}
