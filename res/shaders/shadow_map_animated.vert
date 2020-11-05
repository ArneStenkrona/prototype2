#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    /* Model */
    mat4 model[100];
    mat4 depthVP[4];
    /*Bones*/
    mat4 bones[500];
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelMatrixIdx;
    layout(offset = 4) int cascadeIndex; 
    layout(offset = 36) uint boneOffset;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;
layout(location = 5) in uvec4 inBoneIDs;
layout(location = 6) in vec4 inBoneWeights;

void main() {
    mat4 boneTransform = ubo.bones[inBoneIDs[0] + pc.boneOffset] * inBoneWeights[0];
    boneTransform += ubo.bones[inBoneIDs[1] + pc.boneOffset] * inBoneWeights[1];
    boneTransform += ubo.bones[inBoneIDs[2] + pc.boneOffset] * inBoneWeights[2];
    boneTransform += ubo.bones[inBoneIDs[3] + pc.boneOffset] * inBoneWeights[3];
    vec4 bonedPos = boneTransform * vec4(inPosition, 1.0);

    gl_Position = ubo.depthVP[pc.cascadeIndex] * ubo.model[pc.modelMatrixIdx] * bonedPos;
}
