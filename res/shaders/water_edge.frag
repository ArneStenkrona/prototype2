#version 450
#extension GL_ARB_separate_shader_objects : enable

#define M_PI 3.1415926535897932384626433832795

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

vec4 CalcTexColor(float t, vec2 uv) {
    //float mode; 
    //mode = 0.02 * sin(51.324 * (uv.x + 0.123 * t));
    //mode += 0.03 * sin(30.271 * (uv.x - 0.141 * t));

    //float uvy = uv.y + mode - 0.1;

    //vec2 resUV = vec2(uv.x,  uvy);
    
    //return texture(sampler2D(textures[pc.imgIdx], samp), resUV);

    float f = 0.8;
    f += 0.02 * sin(18 * M_PI * (uv.x + 0.123 * t));
    f += 0.03 * sin(10 * M_PI * (uv.x - 0.141 * t));

    return uv.y > f ? vec4(1.0,1.0,1.0,1.0) : vec4(0.102, 0.247, 0.949, 1.0);
}

vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir) {
    vec4 color = CalcTexColor(fs_in.t, fs_in.fragTexCoord);
    if (color.a == 0.0) {
        discard;
    }
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