#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 fragNormal;
layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform sampler2D texSampler[2];

layout(push_constant) uniform PER_OBJECT
{
	int imgIdx;
}pc;

layout(location = 0) out vec4 outColor;

void main() {
    outColor = texture(texSampler[pc.imgIdx], fragTexCoord);
}
