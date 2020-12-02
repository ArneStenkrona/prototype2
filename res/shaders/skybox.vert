#version 450
#extension GL_ARB_separate_shader_objects : enable

layout (location = 0) in vec3 inPos;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 skyRotation;
	vec4 sunDirection;
	vec4 night;
    vec4 day;
    vec4 sunEdge;
    vec4 sunsetrise;
    vec4 sun;
    float distToNoon;
} ubo;

layout(location = 0) out VS_OUT {
	vec3 fragPos;
	vec3 TexCoords;
} vs_out;

out gl_PerVertex 
{
	vec4 gl_Position;
};

void main() 
{
	vs_out.fragPos = inPos;
	vs_out.fragPos.x *= -1.0;
	vs_out.TexCoords = vec3(ubo.skyRotation * vec4(inPos, 1.0));
	gl_Position = ubo.projection * ubo.model * vec4(inPos, 1.0);
}