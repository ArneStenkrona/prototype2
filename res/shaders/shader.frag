#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec3 fragNormal;
//layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform texture2D textures[10];
layout(set = 0, binding = 2) uniform sampler samp;

layout(set = 0, binding = 0) uniform UniformBufferObject {
    mat4 model[10];
    mat4 view;
    mat4 proj;
    vec3 viewPos;
} ubo;

layout(push_constant) uniform PER_OBJECT
{
	layout(offset = 4) int imgIdx;
}pc;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
} fs_in;

layout(location = 0) out vec4 outColor;

vec3 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir);

void main() {
    vec3 viewDir = normalize(ubo.viewPos - fs_in.fragPos);
    // true=flatshading
    vec3 norm = false ? normalize(cross(dFdx(fs_in.fragPos), dFdy(fs_in.fragPos))) : normalize(fs_in.normal);

    // Directional lighting
    vec3 result = CalcDirLight(vec3(1.0, -1.0, 0.0), norm, viewDir);
    outColor = vec4(result, 1.0);
}

vec3 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir) {
    vec3 color = texture(sampler2D(textures[pc.imgIdx], samp), fs_in.fragTexCoord).rgb;
    vec3 lightDir = normalize(-direction);
    vec3 halfwayDir = normalize(lightDir + viewDir);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);
    // combine results
    vec3 ambient = color * 0.05;
    vec3 diffuse = diff * color;
    vec3 specular = spec * color;
    return (ambient + diffuse + specular);
}
