#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec3 fragNormal;
//layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform texture2D textures[32];
layout(set = 0, binding = 2) uniform sampler samp;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 4) int imgIdx;
} pc;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
    float t;
    vec3 viewDir;
} fs_in;

layout(location = 0) out vec4 outColor;

vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir);

void main() {
    vec3 viewDir = normalize(fs_in.viewDir);
    vec3 norm = normalize(fs_in.normal);

    // Directional lighting
    vec4 result = CalcDirLight(vec3(0.0, -1.0, -1.0), norm, viewDir);
    outColor = result;
}

vec4 CalcColor(float t, vec2 uv) {
    vec2 c = vec2(0.5, 0.5);
    vec2 cuv = uv - c;
    float r = length(cuv);
    float phi = atan(cuv.x,cuv.y);

    float f = (1.5 - r) * pow(sin(50 * (r*r - 0.2 * t)),3) / ((1.0 + 10.0 * r));
    
    return vec4(f,f,f,1.0);
}

vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir) {
    vec4 color = CalcColor(fs_in.t, fs_in.fragTexCoord);
    color = color.r > 0.2 ? vec4(1.0,1.0,1.0,1.0) : vec4(0.102, 0.247, 0.949, 1.0);

    vec3 lightDir = normalize(-direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse shading
    float nDotL = dot(normal, lightDir);

    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

    float levels = 3.0;

    float diffuse = floor( ( (nDotL + 1.0) / 2.0) * levels) / (levels - 1.0);
    float ambient = 1.0f;
    return ((ambient + diffuse) / 2) * color;
}