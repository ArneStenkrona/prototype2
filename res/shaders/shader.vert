#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model[10];
    mat4 view;
    mat4 proj;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelIdx;
}pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;

layout(location = 0) out vec3 fragNormal;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelIdx] * vec4(inPosition, 1.0);
    fragTexCoord = inTexCoord;
}
