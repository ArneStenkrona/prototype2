#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model[10];
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 0) int modelIdx;
}pc;

layout(location = 0) in vec3 inPosition;

void main() {
    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelIdx] * vec4(inPosition, 1.0);
}
