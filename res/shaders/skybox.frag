#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in vec3 TexCoords;

layout(location = 0) out vec4 outColor;

layout(set = 0, binding = 1) uniform samplerCube samplerCubeMap;

void main()
{    
    outColor = texture(samplerCubeMap, TexCoords);
}
