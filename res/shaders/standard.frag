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
    vec4 splitDepths[(4 + 4) / 4];
    mat4 cascadeSpace[4];
    PointLight pointLights[4];
} ubo;

layout(set = 0, binding = 1) uniform texture2D textures[32];
layout(set = 0, binding = 2) uniform sampler samp;

layout(set = 0, binding = 3) uniform sampler2DArray shadowMap;

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
    // vec3 normal;
    vec2 fragTexCoord;
    //float t;
    //vec3 viewDir;
    // vec3 viewPos;
    vec3 shadowPos;
    //mat3 tbn;
    vec3 tangentSunDir;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
    // vec4 sunShadowCoord;
} fs_in;

const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0,
                          0.0, 0.5, 0.0, 0.0,
                          0.0, 0.0, 1.0, 0.0,
                          0.5, 0.5, 0.0, 1.0);


layout(location = 0) out vec4 outColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir,
                    vec3 albedo, float specularity);
vec3 CalcDirLight(vec3 lightDir, vec3 lightColor, vec3 normal, vec3 viewDir,
                  vec3 albedo, float specularity);

float textureProj(vec4 shadowCoord, vec2 off, int cascadeIndex);
float filterPCF(vec4 shadowCoord, int cascadeIndex);
void transparencyDither(float alpha);

void main() {
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

    int cascadeIndex = 0;
    for (int i = 0; i < 4 - 1; ++i) {
        if (fs_in.shadowPos.z < ubo.splitDepths[i/4][i%4]) {
            cascadeIndex = i + 1;
        }
    }
    vec4 sunShadowCoord = (biasMat * ubo.cascadeSpace[cascadeIndex] * vec4(fs_in.fragPos, 1.0));

    // Directional lighting
    res += filterPCF(sunShadowCoord / sunShadowCoord.w, cascadeIndex) *
           CalcDirLight(fs_in.tangentSunDir, ubo.sun.color, normal, viewDir,
                         albedo, specularity);
    outColor = material.normalIndex >= 0 ? vec4(1,1,1,1) : vec4(res, 1.0);
}

// Screen-door transparency: Discard pixel if below threshold.
// (Note: pos is pixel position.)
const mat4 thresholdMatrix = mat4(1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
                                  13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
                                  4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
                                  16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0);

void transparencyDither(float alpha) {
    int x = int(gl_FragCoord.x) / 4;
    int y = int(gl_FragCoord.y) / 4;
    if (alpha - thresholdMatrix[x % 4][y % 4] < 0) discard;
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
    float diff = max(dot(normal, -lightDir), 0.0);
    // specular shading
    // vec3 reflectDir = reflect(-lightDir, normal); // phong
    vec3 halfwayDir = normalize(-lightDir + viewDir); // blinn-phong
    float shininess = 32.0;
    float spec = specularity * pow(max(dot(normal, halfwayDir), 0.0), shininess);
    // combine results
    vec3 diffuse  = lightColor  * diff * albedo;
    vec3 specular = lightColor * spec;
    return diffuse + specular;
}

float textureProj(vec4 shadowCoord, vec2 off, int cascadeIndex) {
    float shadow = 1.0f;
    if (shadowCoord.z > -1.0 && shadowCoord.z < 1.0) {
        float dist = texture(shadowMap, vec3(shadowCoord.st + off, cascadeIndex)).r;
        if (shadowCoord.w > 0.0 && dist < shadowCoord.z) {
            shadow = 0.0f;
        }
    }
    return shadow;
}

float filterPCF(vec4 shadowCoord, int cascadeIndex) {
    ivec2 textDim = textureSize(shadowMap, 0).xy;
    float scale = 1.5;
    float dx = scale * 1.0 / float(textDim.x);
    float dy = scale * 1.0 / float(textDim.y);

    float shadowFactor = 0.0;
    int count = 0;
    int range = 1;

    for (int x = -range; x <= range; ++x) {
        for (int y = -range; y <= range; ++y) {
            shadowFactor += textureProj(shadowCoord, vec2(dx*x, dy*y), cascadeIndex);
            ++count;
        }
    }
    return shadowFactor / count;
}