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
    PointLight pointLights[4];
    /*Bones*/
    mat4 bones[500];
} ubo;

layout(push_constant) uniform PER_OBJECT {
	layout(offset = 0) int modelMatrixIdx;
    layout(offset = 60) uint boneOffset;
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
    vec2 fragTexCoord;
    vec3 fragNormal;
    vec3 shadowPos;
    vec3 tangentSunDir;
    vec3 tangentViewPos;
    vec3 tangentFragPos;
    mat3 invtbn;
} vs_out;

void main() {
    mat4 boneTransform = mat4(1.0);
    float weightSum = inBoneWeights[0] + inBoneWeights[1] + inBoneWeights[2] + inBoneWeights[3];
    if (weightSum > 0.0) { 
        boneTransform  = ubo.bones[inBoneIDs[0] + pc.boneOffset] * inBoneWeights[0];
        boneTransform += ubo.bones[inBoneIDs[1] + pc.boneOffset] * inBoneWeights[1];
        boneTransform += ubo.bones[inBoneIDs[2] + pc.boneOffset] * inBoneWeights[2];
        boneTransform += ubo.bones[inBoneIDs[3] + pc.boneOffset] * inBoneWeights[3];
    }

    vec4 bonedPos = boneTransform * vec4(inPosition, 1.0);

    vs_out.fragPos = vec3(ubo.model[pc.modelMatrixIdx] * bonedPos);

    vec4 boneT = normalize(ubo.invTransposeModel[pc.modelMatrixIdx] * vec4(inTangent, 0.0));
    vec4 boneB = normalize(ubo.invTransposeModel[pc.modelMatrixIdx] * vec4(inBinormal, 0.0));
    vec4 boneN = normalize(ubo.invTransposeModel[pc.modelMatrixIdx] * vec4(inNormal, 0.0));

    vec3 t = normalize(vec3(ubo.invTransposeModel[pc.modelMatrixIdx] * boneT));
    vec3 b = normalize(vec3(ubo.invTransposeModel[pc.modelMatrixIdx] * boneB));
    vec3 n = normalize(vec3(ubo.invTransposeModel[pc.modelMatrixIdx] * boneN));

    vs_out.invtbn = mat3(t,b,n);
    mat3 tbn = transpose(mat3(t,b,n));

    vs_out.fragNormal = n;
    
    vs_out.fragTexCoord = inTexCoord;
    vs_out.shadowPos = (ubo.view * vec4(vs_out.fragPos + n, 1.0)).xyz;

    vs_out.tangentSunDir = tbn * ubo.sun.direction;
    vs_out.tangentViewPos = tbn * ubo.viewPos;
    vs_out.tangentFragPos = tbn * vs_out.fragPos;
    
    gl_Position = ubo.viewProjection * ubo.model[pc.modelMatrixIdx] * bonedPos;
}
