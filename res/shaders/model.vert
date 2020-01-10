#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model[100];
    mat4 invTransposeModel[100];
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelMatrixIdx;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
} vs_out;

void main() {
    vs_out.fragPos = vec3(ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0));
    vs_out.normal = mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * inNormal;
    vs_out.fragTexCoord = inTexCoord;

    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelMatrixIdx] * vec4(inPosition, 1.0);
}
