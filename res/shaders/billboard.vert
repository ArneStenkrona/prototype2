#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    vec4 positions[200];
    vec4 up_vectors[200];
    vec4 right_vectors[200];
    vec4 colors[200];
    mat4 view;
    mat4 projection;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
    layout(offset = 0) vec4 billboardSize;
    layout(offset = 16) int albedoIndex;
    layout(offset = 20) int positionIndex;
} pc;

layout(location = 0) in vec2 inPosition;
layout(location = 1) in vec2 inTexCoord;
// layout(location = 2) in vec3 inNormal;
// layout(location = 3) in vec3 inTangent;
// layout(location = 4) in vec3 inBinormal;

layout(location = 0) out VS_OUT {
    vec3 fragPos;
    vec2 fragTexCoord;
} vs_out;

void main() {
    vec3 vertexPosition_worldspace =
        ubo.positions[pc.positionIndex].xyz
        + ubo.up_vectors[pc.positionIndex].xyz * inPosition.x * pc.billboardSize.x
        + ubo.right_vectors[pc.positionIndex].xyz * inPosition.y * pc.billboardSize.y;

    vs_out.fragPos = vertexPosition_worldspace;
    vs_out.fragTexCoord = inTexCoord;

    gl_Position = ubo.projection * ubo.view * vec4(vertexPosition_worldspace, 1.0);
}
