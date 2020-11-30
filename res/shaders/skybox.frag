#version 450
#extension GL_ARB_separate_shader_objects : enable

// layout(location = 0) in vec3 TexCoords;

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
    vec3 sunDirection;
} ubo;

layout(set = 0, binding = 1) uniform samplerCube samplerCubeMap;

#define PI 3.14159265359

void main()
{    
    vec4 texColor = texture(samplerCubeMap, fs_in.TexCoords);

    float sunSize = 0.02;
    float distToNoon = acos(dot(-ubo.sunDirection, vec3(0,1,0))) / PI;
    /* colors, can be done on CPU */
    vec3 night = mix(vec3(80,80,250), vec3(0,0,0), distToNoon)/255;
    vec3 day = mix(vec3(204,204,255), vec3(5,5,25), distToNoon)/255;
    vec3 sunEdge = vec3(255,119,51)/255;
    vec3 sunsetrise = vec3(255,119,51)/255;
    vec3 sun = mix(vec3(255,255,230), vec3(255,153,51), distToNoon)/255;
    vec3 moon = vec3(128,128,128)/255;

    vec3 dir = normalize(fs_in.fragPos);
    float sunDist = acos(dot(-ubo.sunDirection, dir)) / PI;

    vec3 color; 
    if (sunDist < sunSize) {
        color = sun;
    } else {
        /* angles, converted to [0,1] */
        float theta = (atan(dir.z,dir.x) / (2*PI)) + 0.5;
        float phi = atan(sqrt(dir.x*dir.x+dir.z*dir.z),dir.y) / PI;
        vec2 angle = vec2(theta, phi);

        float distToHorizon = clamp(abs(phi - 0.5) * 2, 0, 1);
        vec3 horizonColor = vec3(clamp(1-10*distToHorizon, 0, 1) * 0.2 * clamp(1-2*distToNoon, 0, 1));

        float sunDistHorizon = acos(dot(normalize(-ubo.sunDirection.xz), normalize(dir.xz))) / PI;
        float sunsetriseDist = abs(distToNoon*2 - 1);
        vec3 sunsetriseColor = clamp(0.5 * clamp(1-7*sunsetriseDist, 0, 1) *
                                     mix(sunsetrise, vec3(0.0, 0.0, 0.0), clamp(20*distToHorizon, 0, 1) + pow(clamp(2*sunDistHorizon, 0, 1), 2))
                                     ,0,1);

        vec3 sunEdgeColor = pow(clamp((1-2*abs(sunDist)), 0, 1),8) * clamp((1-abs(phi - 0.5)), 0, 1) * sunEdge;

        vec3 skyColor = mix(night, day, pow(1-sunDist,2));
        vec3 starColor = (texColor.rgb * texColor.a) * clamp(distToNoon*2 - 1, 0, 1) * clamp(sunDist*2 - 1, 0, 1);
        
        color = sunsetriseColor + sunEdgeColor + skyColor + starColor + horizonColor;
    }

    outColor = vec4(color, 1.0);
}
