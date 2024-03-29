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
    mat4 model[200];
    mat4 invTransposeModel[200];
    mat4 viewProjection;
    mat4 view;
    // mat4 proj;
    vec3 viewPos;
    float t;
    /* Lights */
    float ambientLight;
    uint noPointLights;
    DirLight sun;
    vec4 splitDepths[(5 + 4) / 4];
    mat4 cascadeSpace[5];
    PointLight pointLights[8];
} ubo;

layout(set = 0, binding = 1) uniform texture2D textures[64];
layout(set = 0, binding = 2) uniform sampler samp;

layout(set = 0, binding = 3) uniform sampler2DArray shadowMap;

layout(push_constant) uniform MATERIAL {
	// layout(offset = 0) int modelMatrixIdx;
	layout(offset = 4) int albedoIndex;
	layout(offset = 8) int normalIndex;
	layout(offset = 12) int specularIndex;
    layout(offset = 16) vec4 baseColor;
    layout(offset = 32) float baseSpecularity;
} material;

layout(location = 0) in VS_OUT {
    vec3 fragPos;
    vec2 fragTexCoord;
    vec3 fragNormal;
    vec3 shadowPos;
    vec3 tangentSunDir;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
    mat3 invtbn;
} fs_in;

float linearizeDepth(float d,float zNear,float zFar) {
    return zNear * zFar / (zFar + d * (zNear - zFar));      
}

const mat4 biasMat = mat4(0.5, 0.0, 0.0, 0.0,
                          0.0, 0.5, 0.0, 0.0,
                          0.0, 0.0, 1.0, 0.0,
                          0.5, 0.5, 0.0, 1.0);

layout(location = 0) out vec4 accumColor;
layout(location = 1) out float revealColor;

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir,
                    vec3 albedo, float specularity);
vec3 CalcDirLight(vec3 lightDir, vec3 lightColor, vec3 normal, vec3 viewDir,
                  vec3 albedo, float specularity);

float textureProj(vec4 shadowCoord, vec2 off, int cascadeIndex);
float filterPCF(vec4 shadowCoord, int cascadeIndex);
void transparencyDither(float alpha);   

void main() {
    // get albedo
    float t = ubo.t;
    vec2 tex1 = fs_in.fragTexCoord + vec2(sin(0.3*t), cos(0.25*t));
    float col1 = (material.albedoIndex < 0 ? material.baseColor :
                                        (texture(sampler2D(textures[material.albedoIndex], samp), tex1)) * material.baseColor).r;
    vec2 tex2 = fs_in.fragTexCoord + vec2(sin(0.2*t), cos(0.14*t));
    float col2 = (material.albedoIndex < 0 ? material.baseColor :
                                        (texture(sampler2D(textures[material.albedoIndex], samp), tex2)) * material.baseColor).r;
    float col = mix(col1, col2,0.5);

    if ((col > 0.5 && col < 0.6) || col > 0.96) col = 0;
    vec4 waterColor = vec4(0.4, 0.8, 1, 0.3);
    vec4 albedo = mix(waterColor, vec4(1), col);


    // get specularity
    float specularity = material.specularIndex < 0 ? material.baseSpecularity :
                                    material.baseSpecularity * 2 * texture(sampler2D(textures[material.specularIndex], samp), fs_in.fragTexCoord).r - 1.0;;
    // get normal
    vec3 n1 = material.albedoIndex < 0 ? vec3(0,0,1) :
                                         2.0 * texture(sampler2D(textures[material.albedoIndex], samp), tex1).rgb - 1.0;
    vec3 n2 = material.albedoIndex < 0 ? vec3(0,0,1) :
                                         2.0 * texture(sampler2D(textures[material.albedoIndex], samp), tex2).rgb - 1.0;
    vec3 normal = normalize(mix(vec3(0,0,1), mix(n1,n2,0.5) , 0.2));

    // get view direction
    vec3 viewDir = normalize(fs_in.tangentViewPos - fs_in.tangentFragPos);
    float r = dot(viewDir, normal);
    albedo.a += pow(1.0-r,10.0);


    // light
    vec3 res = ubo.ambientLight * albedo.rgb;
    // Point lights
    for (int i = 0; i < ubo.noPointLights; ++i) {
        res += CalcPointLight(ubo.pointLights[i], normal, viewDir,
                              albedo.rgb, specularity);
    }

    int cascadeIndex = 0;
    for (int i = 0; i < 5 - 1; ++i) {
        if (fs_in.shadowPos.z < ubo.splitDepths[i/4][i%4]) {
            cascadeIndex = i + 1;
        }
    }
    vec4 sunShadowCoord = (biasMat * ubo.cascadeSpace[cascadeIndex] * vec4(fs_in.fragPos + 0.01 * fs_in.fragNormal, 1.0));
    sunShadowCoord = sunShadowCoord / sunShadowCoord.w;
    // Directional lighting
    res += filterPCF(sunShadowCoord, cascadeIndex).r *
           CalcDirLight(fs_in.tangentSunDir, ubo.sun.color, normal, viewDir,
                         albedo.rgb, specularity);
    // transparencyDither(gl_FragCoord.z / gl_FragCoord.w);
    vec4 color = vec4(res, albedo.a);

    // Insert your favorite weighting function here. The color-based factor
    // avoids color pollution from the edges of wispy clouds. The z-based
    // factor gives precedence to nearer surfaces.
    
    float weight = 
            max(min(1.0, max( max(color.r, color.g), color.b ) * color.a ) , color.a) *
            clamp(0.03 / (1e-5 + pow(gl_FragCoord.z / 200, 4.0)), 1e-2, 3e3);

    // Blend Func: GL_ONE, GL_ONE
    // Switch to premultiplied alpha and weight
    
    accumColor = vec4(color.rgb * color.a, color.a) * weight;

    // Blend Func: GL_ZERO, GL_ONE_MINUS_SRC_ALPHA
    revealColor = color.a;
}

// Screen-door transparency: Discard pixel if below threshold.
// (Note: pos is pixel position.)
const mat4 thresholdMatrix = mat4(1.0 / 17.0,  9.0 / 17.0,  3.0 / 17.0, 11.0 / 17.0,
                                  13.0 / 17.0,  5.0 / 17.0, 15.0 / 17.0,  7.0 / 17.0,
                                  4.0 / 17.0, 12.0 / 17.0,  2.0 / 17.0, 10.0 / 17.0,
                                  16.0 / 17.0,  8.0 / 17.0, 14.0 / 17.0,  6.0 / 17.0);

// void transparencyDither(float alpha) {
//     int x = int(gl_FragCoord.x) / 4;
//     int y = int(gl_FragCoord.y) / 4;
//     if (alpha - thresholdMatrix[x % 4][y % 4] < 0) discard;
// }

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 viewDir,
                    vec3 albedo, float specularity) {
    vec3 lightDir = normalize(fs_in.tangentFragPos - transpose(fs_in.invtbn) * light.pos);
    vec3 reflectDir = reflect(lightDir, normal); // phong
    // diffuse shading
    float diff = max(dot(normal, -lightDir), 0.0);
    // specular shading
    float shininess = 8.0;
    float spec = specularity * pow(max(dot(viewDir, reflectDir), 0.0), shininess); 
    // attenuation
    float distance = length(fs_in.tangentFragPos - transpose(fs_in.invtbn) * light.pos);
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
    vec3 reflectDir = reflect(-lightDir, normal); // phong
    vec3 halfwayDir = normalize(-lightDir + viewDir); // blinn-phong

    float shininess = 128.0;
    float spec = 1.0 * pow(max(dot(viewDir, reflectDir), 0.0), shininess);
    // combine results
    vec3 diffuse  = lightColor * diff * albedo;
    vec3 specular = lightColor * spec;
    return diffuse + specular;
}

float textureProj(vec4 shadowCoord, vec2 off, int cascadeIndex) {
    float shadow = 1.0f;

    float dist = texture(shadowMap, vec3(shadowCoord.st + off, cascadeIndex)).r;
    if (shadowCoord.w > 0.0 && dist < shadowCoord.z) {
        shadow = 0.0f;
    }

    return shadow;
}

float filterPCF(vec4 shadowCoord, int cascadeIndex) {
    ivec2 textDim = textureSize(shadowMap, 0).xy;
    float scale = 1.0;
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