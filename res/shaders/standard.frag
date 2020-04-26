#version 450
#extension GL_ARB_separate_shader_objects : enable

struct DirLight {
    vec3 direction;
    vec3 color;
};

struct PointLight {
    vec3 pos;
    float a; // quadtratic term
    vec3 color;
    float b; // linear term
    float c; // constant term
};

layout(set = 0, binding = 0) uniform UniformBufferObject {
    /* Model */
    mat4 model[100];
    mat4 invTransposeModel[100];
    mat4 view;
    mat4 proj;
    vec3 viewPos;
    float t;
    /* Lights */
    float ambientLight;
    int noPointLights;
    DirLight sun;
    mat4 sunSpace;
    PointLight pointLights[4];
} ubo;

layout(set = 0, binding = 1) uniform texture2D textures[32];
layout(set = 0, binding = 2) uniform sampler samp;

layout(set = 0, binding = 3) uniform sampler2D shadowMap;

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
    //vec3 normal;
    vec2 fragTexCoord;
    //float t;
    //vec3 viewDir;
    //vec3 viewPos;
    //mat3 tbn;
    vec3 tangentSunDir;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
    vec4 sunShadowCoord;
} fs_in;

layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir,
                    vec3 albedo, float specularity);
vec3 CalcDirLight(vec3 lightDir, vec3 lightColor, vec3 normal, vec3 viewDir,
                  vec3 albedo, float specularity);

float textureProj(vec4 shadowCoord, vec2 off);

float linearizeDepth(float depth) {
    float n = -10.0f;
    float f = 100.0f;
    float z = depth;
    return (2.0 * n) / (f + n - z * (f - n));
}

void main() {
    // normalize tbn
    //mat3 tbn = mat3(normalize(fs_in.tbn[0]),
    //                normalize(fs_in.tbn[1]),
    //                normalize(fs_in.tbn[2]));

    // get albedo
    vec3 albedo = material.albedoIndex < 0 ? material.baseColor :
                                        //0.5 * (material.baseColor + 2 *
                                        (texture(sampler2D(textures[material.albedoIndex], samp), fs_in.fragTexCoord).rgb);
    // get specularity
    float specularity = material.specularIndex < 0 ? material.baseSpecularity :
                                    material.baseSpecularity * 2 * texture(sampler2D(textures[material.specularIndex], samp), fs_in.fragTexCoord).r - 1.0;;
    // get normal
    vec3 normal = material.normalIndex < 0 ? vec3(0,0,1) :
                                         2.0 * texture(sampler2D(textures[material.normalIndex], samp), fs_in.fragTexCoord).rgb - 1.0;
    normal = normalize(normal);

    // get view direction
    vec3 viewDir = normalize(fs_in.tangentViewPos - fs_in.tangentFragPos);

    // light
    vec3 res = ubo.ambientLight * albedo;;
    // Point lights
    for (int i = 0; i < ubo.noPointLights; ++i) {
        res += CalcPointLight(ubo.pointLights[i], normal, viewDir,
                              albedo, specularity);
    }

    // Directional lighting
    res += textureProj(fs_in.sunShadowCoord / fs_in.sunShadowCoord.w, vec2(0.0)) *
           CalcDirLight(-fs_in.tangentSunDir, ubo.sun.color, normal, viewDir,
                        albedo, specularity);
    outColor = vec4(res, 1.0);
}

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir,
                    vec3 albedo, float specularity) {
    vec3 lightDir = fs_in.tangentFragPos - light.pos;
    vec3 reflectDir = reflect(-lightDir, normal); // phong
    vec3 halfwayDir = normalize(lightDir + viewDir); // blinn-phong
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    float shininess = 32;
    float spec = specularity * pow(max(dot(normal, halfwayDir), 0.0), shininess); 
    // attenuation
    float distance = length(light.pos - fs_in.fragPos);
    float attenuation = 1.0 / (light.c + light.b * distance + light.a * distance * distance);
    // combine result
    vec3 diffuse = light.color * diff * albedo;
    vec3 specular = light.color * spec * specularity;

    diffuse *= attenuation;
    specular *= attenuation;
    return diffuse + specular;
}

vec3 CalcDirLight(vec3 lightDir, vec3 lightColor, vec3 normal, vec3 viewDir,
                  vec3 albedo, float specularity) {
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    // vec3 reflectDir = reflect(-lightDir, normal); // phong
    vec3 halfwayDir = normalize(lightDir + viewDir); // blinn-phong
    float shininess = 32.0;
    float spec = specularity * pow(max(dot(normal, halfwayDir), 0.0), shininess);
    // combine results
    vec3 diffuse  = lightColor  * diff * albedo;
    vec3 specular = lightColor * spec;
    return diffuse + specular;
}

float textureProj(vec4 shadowCoord, vec2 off) {
    float shadow = 1.0f;
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
        float dist = texture(shadowMap, shadowCoord.st + off).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z) {
            shadow = 0.0f;
        }
    }
    return shadow;
}
