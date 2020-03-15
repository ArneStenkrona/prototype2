#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
} ubo;

layout (location = 0) out vec3 TexCoords;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	TexCoords = inPos;
	TexCoords.x *= -1.0;
	gl_Position = ubo.projection * ubo.model * vec4(inPos.xyz, 1.0);
}