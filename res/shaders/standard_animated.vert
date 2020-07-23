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
    /*Bones*/
    mat4 bones[100];
} ubo;

layout(push_constant) uniform PER_OBJECT {
	layout(offset = 0) int modelMatrixIdx;
    layout(offset = 32) uint boneOffset;
} pc;

layout(location = 0) in vec3 inPosition;
layout(location = 1) in vec3 inNormal;
layout(location = 2) in vec2 inTexCoord;
layout(location = 3) in vec3 inTangent;
layout(location = 4) in vec3 inBinormal;
layout(location = 5) in uvec4 inBoneIDs;
layout(location = 6) in vec4 inBoneWeights;

layout(location = 0) out VS_OUT {
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
} vs_out;

void main() {
    mat4 boneTransform = mat4(1.0);
    float weightSum = inBoneWeights[0] + inBoneWeights[1] + inBoneWeights[2] + inBoneWeights[3];
    if (weightSum > 0.0) { 
        boneTransform  = ubo.bones[inBoneIDs[0]] * inBoneWeights[0];
        boneTransform += ubo.bones[inBoneIDs[1] + pc.boneOffset] * inBoneWeights[1];
        boneTransform += ubo.bones[inBoneIDs[2] + pc.boneOffset] * inBoneWeights[2];
        boneTransform += ubo.bones[inBoneIDs[3] + pc.boneOffset] * inBoneWeights[3];
    }

    vec4 bonedPos = boneTransform * vec4(inPosition, 1.0);

    vs_out.fragPos = vec3(ubo.model[pc.modelMatrixIdx] * bonedPos);

    vec3 boneT = normalize(mat3(inverse(transpose(boneTransform))) * inTangent);
    vec3 boneB = normalize(mat3(inverse(transpose(boneTransform))) * inBinormal);
    vec3 boneN = normalize(mat3(inverse(transpose(boneTransform))) * inNormal);

    vec3 t = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * boneT);
    vec3 b = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * boneB);
    vec3 n = normalize(mat3(ubo.invTransposeModel[pc.modelMatrixIdx]) * boneN);

    mat3 tbn = transpose(mat3(t,b,n));
    
    vs_out.fragTexCoord = inTexCoord;
    vs_out.shadowPos = (ubo.view * vec4(vs_out.fragPos + n, 1.0)).xyz;

    vs_out.tangentSunDir = tbn * ubo.sun.direction;
    vs_out.tangentViewPos = tbn * ubo.viewPos;
    vs_out.tangentFragPos = tbn * vs_out.fragPos;
    
    gl_Position = ubo.proj * ubo.view * ubo.model[pc.modelMatrixIdx] * bonedPos;
}
