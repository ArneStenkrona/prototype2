#version 450
#extension GL_ARB_separate_shader_objects : enable

layout(location = 0) in VS_OUT {
	vec3 fragPos;
	vec3 TexCoords;
} fs_in;

layout(location = 0) out vec4 outColor;

layout (binding = 0) uniform UBO 
{
	mat4 projection;
	mat4 model;
	mat4 skyRotation;
	vec4 sunDirection;
    vec4 nightColor;
    vec4 dayColor;
    vec4 sunEdgeColor;
    vec4 sunsetriseColor;
    vec4 sunColor;
    float distToNoon;
} ubo;

layout(set = 0, binding = 1) uniform samplerCube samplerCubeMap;
layout(location = 1) out int outObject;

#define PI 3.14159265359

void main()
{    
    vec4 texColor = texture(samplerCubeMap, fs_in.TexCoords);

    float sunSize = 0.02;

    vec3 dir = normalize(fs_in.fragPos);
    float sunDist = acos(dot(-ubo.sunDirection.xyz, dir)) / PI;

    vec3 color; 
    if (sunDist < sunSize) {
        color = ubo.sunColor.rgb;
    } else {
        /* angles, converted to [0,1] */
        float theta = (atan(dir.z,dir.x) / (2*PI)) + 0.5;
        float phi = atan(sqrt(dir.x*dir.x+dir.z*dir.z),dir.y) / PI;
        vec2 angle = vec2(theta, phi);

        float distToHorizon = clamp(abs(phi - 0.5) * 2, 0, 1);
        vec3 horizonColor = vec3(clamp(1-10*distToHorizon, 0, 1) * 0.2 * clamp(1-2*ubo.distToNoon, 0, 1));

        float sunDistHorizon = acos(dot(normalize(-ubo.sunDirection.xz), normalize(dir.xz))) / PI;
        float sunsetriseDist = abs(ubo.distToNoon*2 - 1);
        vec3 sunsetriseColor = clamp(0.5 * clamp(1-7*sunsetriseDist, 0, 1) *
                                     mix(ubo.sunsetriseColor.rgb, vec3(0.0, 0.0, 0.0), clamp(20*distToHorizon, 0, 1) + pow(clamp(2*sunDistHorizon, 0, 1), 2))
                                     ,0,1);

        vec3 sunEdgeColor = pow(clamp((1-2*abs(sunDist)), 0, 1),8) * clamp((1-abs(phi - 0.5)), 0, 1) * ubo.sunEdgeColor.rgb;

        vec3 skyColor = mix(ubo.nightColor.rgb, ubo.dayColor.rgb, pow(1-sunDist,2));
        vec3 starColor = (texColor.rgb * texColor.a) * clamp(ubo.distToNoon*2 - 1, 0, 1) * clamp(sunDist*2 - 1, 0, 1);
        
        color = sunsetriseColor + sunEdgeColor + skyColor + starColor + horizonColor;
    }

    outColor = vec4(color, 1.0);
    outObject = -1;
}
