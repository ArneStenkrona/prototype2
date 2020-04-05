#version 450
#extension GL_ARB_separate_shader_objects : enable

//layout(location = 0) in vec3 fragNormal;
//layout(location = 1) in vec2 fragTexCoord;

layout(set = 0, binding = 1) uniform texture2D albedoMaps[32];
layout(set = 0, binding = 2) uniform texture2D normalMaps[32];
layout(set = 0, binding = 3) uniform texture2D specularMaps[32];

layout(set = 0, binding = 4) uniform sampler samp;


struct PointLight {
    vec3 pos;

    float a; // quadtratic term
    float b; // linear term
    float c; // constant term

    vec3 color;
};

layout(set = 0, binding = 5) uniform Lights {
    int noPointLights;
    PointLight pointLights[4];//@NUMBER_SUPPORTED_POINTLIGHTS@];
};

layout(push_constant) uniform MATERIAL {
	// layout(offset = 0) int modelMatrixIdx;
	layout(offset = 4) int albedoIndex;
	layout(offset = 8) int normalIndex;
	layout(offset = 12) int specularIndex;
    layout(offset = 16) vec3 baseColor;
    layout(offset = 28) float baseSpecularity;
} material;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec3 normal;
    vec2 fragTexCoord;
    float t;
    vec3 viewDir;
    mat3 tbn;
} fs_in;


layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir);
vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir);

void main() {
    vec3 normal = material.normalIndex < 0 ? fs_in.normal :
                                    2 * texture(sampler2D(normalMaps[material.normalIndex], samp), fs_in.fragTexCoord).rgb - 1.0;
    normal = normalize(normal);

    vec3 viewDir = fs_in.tbn * normalize(fs_in.viewDir);
    vec3 res;
    // Point lights
    for (int i = 0; i < noPointLights; ++i) {
        res += CalcPointLight(pointLights[i], normal, viewDir);
    }

    // Directional lighting
    // vec4 result = CalcDirLight(vec3(0.0, -1.0, -1.0), normal, viewDir);
    outColor = vec4(res, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(light.pos - fs_in.fragPos);
    vec3 halfwayDir = normalize(lightDir + viewDir);    
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float specularity = material.specularIndex < 0 ? material.baseSpecularity :
                                    material.baseSpecularity * 2 * texture(sampler2D(specularMaps[material.specularIndex], samp), fs_in.fragTexCoord).r - 1.0;
    float spec = pow(max(dot(normal, halfwayDir), 0.0), specularity);
    // attenuation
    float distance = length(light.pos - fs_in.fragPos);
    float attenuation = 1.0 / (light.c + light.b * distance + light.a * distance * distance);
    // combine result
    vec3 albedo = material.albedoIndex < 0 ? material.baseColor :
                                        0.5 * (material.baseColor +
                                               2 * texture(sampler2D(normalMaps[material.albedoIndex], samp), fs_in.fragTexCoord).rgb - 1.0);
    vec3 ambient = light.color * albedo;
    vec3 diffuse = light.color * diff * albedo;
    vec3 specular = light.color * spec * specularity;

    ambient *= attenuation;
    diffuse *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}

// vec4 CalcDirLight(vec3 direction, vec3 normal, vec3 viewDir) {
//     vec4 color = texture(sampler2D(textures[pc.imgIdx], samp), fs_in.fragTexCoord);
//     if (color.a == 0.0) {
//         discard;
//     }
//     vec3 lightDir = normalize(-direction);
//     vec3 halfwayDir = normalize(lightDir + viewDir);
//     // diffuse shading
//     float nDotL = dot(normal, lightDir);

//     vec3 reflectDir = reflect(-lightDir, normal);
//     float spec = pow(max(dot(normal, halfwayDir), 0.0), 32.0);

//     float levels = 3.0;
    
//     float diffuse = floor( ( (nDotL + 1.0) / 2.0) * levels) / (levels - 1.0);
//     float ambient = 1.0f;
//     return ((ambient + diffuse) / 2) * color;
// }
