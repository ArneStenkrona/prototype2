#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(push_constant) uniform PER_OBJECT
{
layout(offset = 4) uint r;
layout(offset = 8) uint g;
layout(offset = 16) uint b;
} pushConsts;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = vec4(pushConsts.r/255.f, pushConsts.g/255.f, pushConsts.b/255.f, 1.0f);
}