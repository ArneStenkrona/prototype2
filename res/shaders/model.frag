#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec3 fragNormal;
//layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform texture2D textures[20];
layout(set = 0, binding = 2) uniform sampler samp;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 4) int imgIdx;
} pc;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
    vec3 viewDir;
} fs_in;

layout(location = 0) out vec4 outColor;

vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir);

void main() {
    vec3 viewDir = normalize(fs_in.viewDir);
    // true=flatshading
    vec3 norm = normalize(fs_in.normal);

    // Directional lighting
    vec4 result = CalcDirLight(vec3(0.0, -1.0, -1.0), norm, viewDir);
    outColor = result;
}

vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir) {
    vec4 color = texture(sampler2D(textures[pc.imgIdx], samp), fs_in.fragTexCoord);//.rgb;
    if (color.a <= 0.1) {
        discard;
    }
    vec3 lightDir = normalize(-direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    // combine results
    vec4 ambient = color * 0.05;
    vec4 diffuse = diff * color;
    vec4 specular = spec * color;
    return (ambient + diffuse + specular);
}
